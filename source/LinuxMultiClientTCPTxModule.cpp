#include "LinuxMultiClientTCPTxModule.h"

LinuxMultiClientTCPTxModule::LinuxMultiClientTCPTxModule(std::string sIPAddress, uint16_t u16TCPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) : BaseModule(uMaxInputBufferSize),
																																								m_sDestinationIPAddress(sIPAddress),
																																								m_u16TCPPort(u16TCPPort),
																																								m_bTCPConnected()
{
}

LinuxMultiClientTCPTxModule::~LinuxMultiClientTCPTxModule()
{
}

void LinuxMultiClientTCPTxModule::ConnectTCPSocket(int &sock, uint16_t u16TCPPort)
{
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1)
	{
		std::string strFatal = std::string(__FUNCTION__) + ":INVALID_SOCKET ";
		PLOG_FATAL << strFatal;
		// Handle the error here
		// throw or return an error code
	}

	// Allow for multiple binds to single address
	int optval = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

	// Forces socket to not timeout
	int optlen = sizeof(optval);
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, optlen);

	// Prevents crashes when server closes ubruptly and casues sends to fail
	signal(SIGPIPE, SIG_IGN);

	sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(u16TCPPort);
	sockaddr.sin_addr.s_addr = inet_addr(m_sDestinationIPAddress.c_str());

	std::string strInfo = std::string(__FUNCTION__) + ": Connecting to Server at ip " + m_sDestinationIPAddress + " on port " + std::to_string(u16TCPPort) + "";
	PLOG_INFO << strInfo;

	// Set socket timeout for connect
	struct timeval timeout;
	timeout.tv_sec = 5;  // Set timeout to 5 seconds
	timeout.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

	auto iConnectResult = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	if (iConnectResult == 0)
	{
		std::string strInfo = std::string(__FUNCTION__) + ": Connected to server at ip " + m_sDestinationIPAddress + " on port " + std::to_string(u16TCPPort) + "";
		PLOG_INFO << strInfo;
		m_bTCPConnected = true;
	}
	else
	{
		std::string strWarning = std::string(__FUNCTION__) + ": Failed to connect to the server(" + m_sDestinationIPAddress + ") on port " + std::to_string(u16TCPPort) + ".Error code :" + std::to_string(errno);
		PLOG_WARNING << strWarning;

		close(sock);
		m_bTCPConnected = false;

		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	}
}

uint16_t LinuxMultiClientTCPTxModule::WaitForReturnedPortAllocation(int &WinSocket)
{

	std::vector<char> vcAccumulatedBytes;
	vcAccumulatedBytes.reserve(sizeof(uint16_t));
	bool bReadError;

	// Wait for data to be available on the socket
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(WinSocket, &readfds);
	int num_ready = select(WinSocket + 1, &readfds, NULL, NULL, NULL);

	if (num_ready < 0)
	{
		std::string strWarning = std::string(__FUNCTION__) + ": Failed to wait for data on socket ";
		PLOG_WARNING << strWarning;
	}

	// Read the data from the socket
	if (FD_ISSET(WinSocket, &readfds))
	{

		// Arbitrarily using 2048 and 512
		while (vcAccumulatedBytes.size() < sizeof(uint16_t))
		{

			std::vector<char> vcByteData;
			vcByteData.resize(sizeof(uint16_t));
			int16_t i16ReceivedDataLength = recv(WinSocket, &vcByteData[0], sizeof(uint16_t), 0);

			bReadError = CheckForSocketReadErrors(i16ReceivedDataLength,vcByteData.size());
			if(bReadError)
				break;

			for (int i = 0; i < i16ReceivedDataLength; i++)
				vcAccumulatedBytes.emplace_back(vcByteData[i]);
		}
	}
	
	uint16_t u16AllocatedPortNumber;
	memcpy(&u16AllocatedPortNumber, &vcAccumulatedBytes[0], sizeof(uint16_t));
	LOG_WARNING << "Client allocated port " + std::to_string(u16AllocatedPortNumber);

	return u16AllocatedPortNumber;
}

void LinuxMultiClientTCPTxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	while (!m_bShutDown)
	{
		if (!m_bTCPConnected)
		{
			int AllocatingServerSocket;

			// Lets request a port number on which to communicate with the server
			ConnectTCPSocket(AllocatingServerSocket, m_u16TCPPort);

			if (!m_bTCPConnected)
			{
				// Could not connect so wait a bit as not to spam logs
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

			auto u16AllocatedPortNumber = WaitForReturnedPortAllocation(AllocatingServerSocket);
			DisconnectTCPSocket(AllocatingServerSocket);

			// Now that we got it, let open that port and stream data
			int AllocatedServerSocket;
			ConnectTCPSocket(AllocatedServerSocket, u16AllocatedPortNumber);
			std::thread clientThread([this, &AllocatedServerSocket, u16AllocatedPortNumber]
									 { RunClientThread(AllocatedServerSocket, u16AllocatedPortNumber); });
			clientThread.detach();
		}
		else
		{
			// While we are already connected lets just put the thread to sleep
			std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		}
	}
}

void LinuxMultiClientTCPTxModule::RunClientThread(int &clientSocket, uint16_t u16AllocatedPortNumber)
{
	while (!m_bShutDown)
	{
		try
		{
			// During processing we see if there is data (UDP Chunk) in the input buffer
			std::shared_ptr<BaseChunk> pBaseChunk;
			if (TakeFromBuffer(pBaseChunk))
			{
				// Cast it back to a UDP chunk
				auto pByteChunk = std::static_pointer_cast<ByteChunk>(pBaseChunk);
				const auto pvcData = pByteChunk->m_vcDataChunk;
				size_t length = pvcData.size();

				// And then transmit (wohoo!!!)
				int bytes_sent = send(clientSocket, &pvcData[0], length, 0);
				if (bytes_sent < 0)
				{
					std::string strInfo = std::string(__FUNCTION__) + ": No data transmitted on port " + std::to_string(u16AllocatedPortNumber);
					PLOG_INFO << strInfo;
					break;
				}
			}
			else
			{
				// Wait to be notified that there is data available
				std::unique_lock<std::mutex> BufferAccessLock(m_BufferStateMutex);
				m_cvDataInBuffer.wait(BufferAccessLock, [this]
									  { return (!m_cbBaseChunkBuffer.empty() || m_bShutDown); });
			}
		}
		catch (const std::exception &e)
		{
			std::string strError = "Client thread error: " + std::string(e.what());
			PLOG_ERROR << strError;
			break;
		}
	}

	// In the case of stopping processing or an error we
	// formally close the socket and update state variable
	std::string strInfo = std::string(__FUNCTION__) + ": Closing TCP Socket at ip " + m_sDestinationIPAddress + " on port " + std::to_string(u16AllocatedPortNumber);
	PLOG_INFO << strInfo;

	DisconnectTCPSocket(clientSocket);
	m_bTCPConnected = false;
}

void LinuxMultiClientTCPTxModule::DisconnectTCPSocket(int &clientSocket)
{
	close(clientSocket);
	// WSACleanup();
}

void LinuxMultiClientTCPTxModule::StartProcessing()
{
	// Passing in empty chunk that is not used
	m_thread = std::thread([this]
						   { Process(std::shared_ptr<BaseChunk>()); });
}

void LinuxMultiClientTCPTxModule::ContinuouslyTryProcess()
{
	// Passing in empty chunk that is not used
	m_thread = std::thread([this]
						   { Process(std::shared_ptr<BaseChunk>()); });
}


bool LinuxMultiClientTCPTxModule::CheckForSocketReadErrors(ssize_t stReportedSocketDataLength, size_t stActualDataLength)
{
     // Check for timeout
    if (stReportedSocketDataLength == -1 && errno == EAGAIN)
    {
        std::string strWarning = std::string(__FUNCTION__) + ": recv() timed out  or errored for client (error: " + std::to_string(errno) + ") " + m_sDestinationIPAddress;
        PLOG_WARNING << strWarning;
        return true;
    }

    else if (stReportedSocketDataLength == 0)
    {
        // connection closed, too handle
        std::string strInfo = std::string(__FUNCTION__) + ": client " + m_sDestinationIPAddress + " closed connection closed, shutting down thread";
        PLOG_INFO << strInfo;
        return true;
    }

    // And then try store data
    if (stReportedSocketDataLength > stActualDataLength)
    {
        std::string strWarning = std::string(__FUNCTION__) + ": Closed connection to " + std::to_string(m_u16TCPPort) + ": received data length shorter than actual received data ";
        PLOG_WARNING << strWarning;
        return true;
    }

    return false;
}

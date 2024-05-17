#include "LinuxTCPTxModule.h"

LinuxTCPTxModule::LinuxTCPTxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) : BaseModule(uMaxInputBufferSize),
                                                                                                                                          m_sDestinationIPAddress(sIPAddress),
                                                                                                                                          m_sTCPPort(sTCPPort),
                                                                                                                                          m_WinSocket(),
                                                                                                                                          m_SocketStruct(),
                                                                                                                                          m_bTCPConnected()
{
}

LinuxTCPTxModule::~LinuxTCPTxModule()
{
    CloseTCPSocket(m_WinSocket);
}

void LinuxTCPTxModule::ConnectTCPSocket()
{
    // Configuring protocol to TCP
    if ((m_WinSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        std::string strError = std::string(__FUNCTION__) + ":Windows TCP socket WSA Error. INVALID_int ";
        PLOG_ERROR << strError;
        throw;
    }

    // Allow address reuse
    int optval = 1;
    if (setsockopt(m_WinSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0)
    {
        // Handle error
    }

    // Keep the connection open
    int optlen = sizeof(optval);
    if (setsockopt(m_WinSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, optlen) < 0)
    {
        return;
    }
}

void LinuxTCPTxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Constantly looking for new connections and stating client threads
    // One thread should be created at a time, corresponding to one simulated device
    // In the case of an error, the thread will close and this will recreate the socket
    while (!m_bShutDown)
    {
        if (!m_bTCPConnected)
        {
            ConnectTCPSocket();

            std::string strInfo = std::string(__FUNCTION__) + ": Connecting to Server at ip " + m_sDestinationIPAddress + " on port " + m_sTCPPort + "";
            PLOG_INFO << strInfo;

            // Lets start by creating the sock addr
            sockaddr_in sockaddr;
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_port = htons(stoi(m_sTCPPort));

            // Lets then convert an IPv4 or IPv6 to its binary representation
            if (inet_pton(AF_INET, m_sDestinationIPAddress.c_str(), &(sockaddr.sin_addr)) <= 0)
            {
                std::string strWarning = std::string(__FUNCTION__) + ": Invalid IP address ";
                PLOG_WARNING << strWarning;
                return;
            }

            auto clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket == -1)
            {
                std::string strFatal = std::string(__FUNCTION__) + ":INVALID_SOCKET ";
                PLOG_FATAL << strFatal;
                // Handle the error here
                // throw or return an error code
            }

            // Then lets do a blocking call to try connect
            if (connect(clientSocket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == 0)
            {
                std::string strInfo = std::string(__FUNCTION__) + ": Connected to server at ip " + m_sDestinationIPAddress + " on port " + m_sTCPPort + "";
                PLOG_INFO << strInfo;

                // And update connection state and spin of the processing thread
                m_bTCPConnected = true;
                std::thread clientThread([this, &clientSocket]
                                         { RunClientThread(clientSocket); });
                clientThread.detach();
            }
            else
            {
                std::string strWarning = std::string(__FUNCTION__) + ": Failed to connect to the server.Error code :" + std::to_string(errno);
                PLOG_WARNING << strWarning;
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                close(clientSocket);
            }
        }
        else
        {
            // While we are already connected lets just put the thread to sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

void LinuxTCPTxModule::RunClientThread(int &clientSocket)
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
                auto udpChunk = std::static_pointer_cast<ByteChunk>(pBaseChunk);
                const auto pvcData = udpChunk->m_vcDataChunk;
                size_t length = pvcData.size();

                // And then transmit (wohoo!!!)
                int bytes_sent = send(clientSocket, &pvcData[0], length, 0);
                if (bytes_sent < 0)
                {
                    // An error occurred.
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
            std::cout << e.what() << std::endl;
            break;
        }
    }

    // In the case of stopping processing or an error we
    // formally close the socket and update state variable
    std::string strInfo = std::string(__FUNCTION__) + ": Closing TCP Socket at ip " + m_sDestinationIPAddress + " on port " + m_sTCPPort + "";
    PLOG_INFO << strInfo;

    CloseTCPSocket(clientSocket);
    m_bTCPConnected = false;
}

void LinuxTCPTxModule::CloseTCPSocket(int &clientSocket)
{
    close(clientSocket);
}

void LinuxTCPTxModule::StartProcessing()
{
    // Passing in empty chunk that is not used
    m_thread = std::thread([this]
                           { Process(std::shared_ptr<BaseChunk>()); });
}

void LinuxTCPTxModule::ContinuouslyTryProcess()
{
    // Passing in empty chunk that is not used
    m_thread = std::thread([this]
                           { Process(std::shared_ptr<BaseChunk>()); });
}
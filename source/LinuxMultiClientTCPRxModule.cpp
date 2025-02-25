#include "LinuxMultiClientTCPRxModule.h"

LinuxMultiClientTCPRxModule::LinuxMultiClientTCPRxModule(std::string sIPAddress, uint16_t u16TCPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) : BaseModule(uMaxInputBufferSize),
                                                                                                                                                                m_sIPAddress(sIPAddress),
                                                                                                                                                                m_u16TCPPort(u16TCPPort),
                                                                                                                                                                m_iDatagramSize(iDatagramSize),
                                                                                                                                                                m_u16LifeTimeConnectionCount(0)
{
}

LinuxMultiClientTCPRxModule::~LinuxMultiClientTCPRxModule()
{
}

void LinuxMultiClientTCPRxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    while (!m_bShutDown)
    {
        // Connect to the allocation port and start listening for client to connections
        int AllocatingServerSocket;

        ConnectTCPSocket(AllocatingServerSocket, m_u16TCPPort);
        AllocateAndStartClientProcess(AllocatingServerSocket);
        CloseTCPSocket(AllocatingServerSocket);
    }
}

void LinuxMultiClientTCPRxModule::ConnectTCPSocket(int &sock, uint16_t u16TCPPort)
{

    // Configuring protocol to TCP
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
    {
        std::string strError = "Windows TCP socket WSA Error. INVALID_SOCKET ";
        PLOG_ERROR << strError;
        throw;
    }

    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

    int optlen = sizeof(optval);
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, optlen);

    // Bind the socket to a local IP address and port number
    sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any local IP address
    localAddr.sin_port = htons(u16TCPPort); //

    if (bind(sock, (sockaddr *)&localAddr, sizeof(localAddr)) < 0)
    {
        std::string strError = std::string(__FUNCTION__) + ": Bind failed ";
        PLOG_ERROR << strError;
        throw;
    }

    // Set the socket to blocking mode
    // u_long mode = 0; // 0 for blocking, non-zero for non-blocking
    // if (ioctlsocket(sock, FIONBIO, &mode) == SOCKET_ERROR)
    // {
    //     std::string strError = std::string(__FUNCTION__) + " ioctlsocket failed with error: " + std::to_string(WSAGetLastError());
    //     PLOG_ERROR << strError;

    //     closesocket(sock);
    //     WSACleanup();
    //     return;
    // }

    // Start listening on socket
    if (listen(sock, SOMAXCONN) < 0)
    {
        std::string strError = std::string(__FUNCTION__) + ": Error listening on server socket. Error code: " + std::to_string(errno) + " ";
        PLOG_ERROR << strError;

        close(sock);
        throw;
    }

    std::string strInfo = std::string(__FUNCTION__) + ": Socket binding complete, listening on IP: " + m_sIPAddress + " and port " + std::to_string(u16TCPPort) + " ";
    PLOG_INFO << strInfo;
}

void LinuxMultiClientTCPRxModule::AllocateAndStartClientProcess(int &AllocatingServerSocket)
{
    {
        std::string strInfo = std::string(__FUNCTION__) + ": Waiting for client connection requests";
        PLOG_INFO << strInfo;
    }

    int PortNumberAllcationSocket = accept(AllocatingServerSocket, NULL, NULL);
    if (PortNumberAllcationSocket < 0)
    {
        std::string strWarning = std::string(__FUNCTION__) + ": Error accepting client connection. Error code: " + std::to_string(errno) + "";
        PLOG_WARNING << strWarning;
        return;
    }

    std::string strInfo = std::string(__FUNCTION__) + ": Accepted client connection. Client socket: " + std::to_string(PortNumberAllcationSocket) + "";
    PLOG_INFO << strInfo;

    // Increment and define port we can allocate to new client to not have port clash
    m_u16LifeTimeConnectionCount += 1;
    uint16_t u16AllocatedPortNumber = m_u16TCPPort + m_u16LifeTimeConnectionCount;

    {
        std::string strInfo = std::string(__FUNCTION__) + ": Begining client port allocation to port " + std::to_string(u16AllocatedPortNumber);
        PLOG_INFO << strInfo;
    }

    // We now spin up a new thread to handle the allocated client connection
    std::thread clientThread([this, u16AllocatedPortNumber]
                             { StartClientThread(u16AllocatedPortNumber); });
    clientThread.detach();

    PLOG_INFO << "-----";

    // And once the thread is operation we transmit the port the thread uses to the client
    auto vcData = std::vector<char>(sizeof(u16AllocatedPortNumber));
    memcpy(&vcData[0], &u16AllocatedPortNumber, sizeof(u16AllocatedPortNumber));

    int bytes_sent = send(PortNumberAllcationSocket, &vcData[0], vcData.size(), 0);
    if (bytes_sent < 0)
    {
        // The client should close the connection once it has figured out the port it should use
        throw;
    }

    {
        std::string strInfo = std::string(__FUNCTION__) + ": Allocated client to port " + std::to_string(u16AllocatedPortNumber);
        PLOG_INFO << strInfo;
    }

    CloseTCPSocket(PortNumberAllcationSocket);
}

void LinuxMultiClientTCPRxModule::StartClientThread(uint16_t u16AllocatedPortNumber)
{
    int InitialClientConnectionSocket;
    ConnectTCPSocket(InitialClientConnectionSocket, u16AllocatedPortNumber);
    int clientSocket = accept(InitialClientConnectionSocket, NULL, NULL);

    // New code to get the client IP address
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    std::string clientIP = "NO IP determined";
    if (getpeername(clientSocket, (sockaddr*)&clientAddr, &clientAddrLen) == 0) {
        clientIP = inet_ntoa(clientAddr.sin_addr);
        std::string strInfo = std::string(__FUNCTION__) + ": Client connected from IP: " + clientIP;
        PLOG_INFO << strInfo;
    } else {
        PLOG_WARNING << "Failed to get client IP address.";
    }

    {
        std::string strInfo = std::string(__FUNCTION__) + ": Starting client thread ";
        PLOG_INFO << strInfo;
    }

    std::vector<char> vcAccumulatedBytes;
    vcAccumulatedBytes.reserve(2048);

    // Set a timeout for the recv function
    struct timeval recvTimeout;
    recvTimeout.tv_sec = 5; // seconds
    recvTimeout.tv_usec = 0; // microseconds
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));

    bool bSocketErrorOccured = false;

    while (!m_bShutDown)
    {
        // Wait for data to be available on the socket
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        // Set a timeout of 5 seconds
        struct timeval timeout;
        timeout.tv_sec = 10; // seconds
        timeout.tv_usec = 0; // microseconds

        int num_ready = select(clientSocket + 1, &readfds, NULL, NULL, &timeout);

        if (num_ready < 0)
        {
            std::string strWarning = std::string(__FUNCTION__) + ": Failed to wait for data on socket for client " + clientIP +" with error: " + std::to_string(errno);
            PLOG_WARNING << strWarning;
            break; // Exit the loop on error
        }
        else if (num_ready == 0)
        {
            std::string strWarning = std::string(__FUNCTION__) + ": Timeout occured after 10s for client " + clientIP;
            PLOG_WARNING << strWarning;
            break; // Exit the loop on error
        }

        // Read the data from the socket
        if (FD_ISSET(clientSocket, &readfds))
        {   
            // Arbitrarily using 2048 and 512
            bool bSocketErrorOccured = false;
            while (vcAccumulatedBytes.size() < 512)
            {
                std::vector<char> vcByteData;
                vcByteData.resize(512);
                int uReceivedDataLength = recv(clientSocket, &vcByteData[0], 512, 0);

                bSocketErrorOccured = CheckForSocketReadErrors(uReceivedDataLength, vcByteData.size());
                if(bSocketErrorOccured)
                    break;

                for (int i = 0; i < uReceivedDataLength; i++)
                    vcAccumulatedBytes.emplace_back(vcByteData[i]);
            }

            if(bSocketErrorOccured)
                    break;
                    
            // Now see if a complete object has been passed on socket
            // Search for null character
            uint16_t u16SessionTransmissionSize;
            memcpy(&u16SessionTransmissionSize, &vcAccumulatedBytes[0], 2);

            // Lets now extract the bytes realting to the received class
            auto vcCompleteClassByteVector = std::vector<char>(vcAccumulatedBytes.begin() + 2, vcAccumulatedBytes.begin() + sizeof(u16SessionTransmissionSize) + u16SessionTransmissionSize);

            // And then create a vector of the remaining bytes to carry on processing with
            auto vcReducedAccumulatedBytes = std::vector<char>(vcAccumulatedBytes.begin() + u16SessionTransmissionSize, vcAccumulatedBytes.end());
            vcAccumulatedBytes = vcReducedAccumulatedBytes;

            // And then store it in the generic class for futher processing
            auto pUDPDataChunk = std::make_shared<ByteChunk>(m_iDatagramSize);
            pUDPDataChunk->m_vcDataChunk = vcCompleteClassByteVector;

            TryPassChunk(std::dynamic_pointer_cast<BaseChunk>(pUDPDataChunk));

        }
   
    }

    CloseTCPSocket(clientSocket);
}

void LinuxMultiClientTCPRxModule::CloseTCPSocket(int socket)
{
    close(socket);
}

void LinuxMultiClientTCPRxModule::StartProcessing()
{
    // passing in empty chunk that is not used
    m_thread = std::thread([this]
                           { Process(std::shared_ptr<BaseChunk>()); });
}

void LinuxMultiClientTCPRxModule::ContinuouslyTryProcess()
{
    // passing in empty chunk that is not used
    m_thread = std::thread([this]
                           { Process(std::shared_ptr<BaseChunk>()); });
}


bool LinuxMultiClientTCPRxModule::CheckForSocketReadErrors(ssize_t stReportedSocketDataLength, size_t stActualDataLength)
{
     // Check for timeout
    if (stReportedSocketDataLength == -1 && errno == EAGAIN)
    {
        std::string strWarning = std::string(__FUNCTION__) + ": recv() timed out  or errored for client (error: " + std::to_string(errno) + ") " + m_sIPAddress;
        PLOG_WARNING << strWarning;
        return true;
    }

    else if (stReportedSocketDataLength == 0)
    {
        // connection closed, too handle
        std::string strInfo = std::string(__FUNCTION__) + ": client " + m_sIPAddress + " closed connection closed, shutting down thread";
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
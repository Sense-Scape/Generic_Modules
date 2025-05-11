#include "TCPRxModule.h"

TCPRxModule::TCPRxModule(std::string sIPAddress, uint16_t u16TCPPort, unsigned uMaxInputBufferSize, int iDatagramSize, std::string strMode)
    : BaseModule(uMaxInputBufferSize),
      m_sBindIPAddress(sIPAddress),
      m_u16TCPPort(u16TCPPort),
      m_Socket(),
      m_SocketStruct(),
      m_bTCPConnected(),
      m_strMode(strMode),
      m_iDatagramSize(iDatagramSize)
{
}

TCPRxModule::~TCPRxModule()
{
    close(m_Socket);
}

void TCPRxModule::Process()
{
    if (m_strMode == std::string("Connect")) {
        ConnectToServer();
    } else if (m_strMode == std::string("Listen")) {
        ListenForClients();
    } else {
        PLOG_ERROR << std::string(__FUNCTION__) + ": Invalid Connection Mode";
        throw;
    }
}

void TCPRxModule::ConnectToServer()
{
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(m_u16TCPPort);
    signal(SIGPIPE, SIG_IGN);

    while (!m_bShutDown)
    {
        if (!m_bTCPConnected)
        {
            if (inet_pton(AF_INET, m_sBindIPAddress.c_str(), &(sockaddr.sin_addr)) <= 0)
            {
                PLOG_WARNING << std::string(__FUNCTION__) + ": Invalid IP address ";
                return;
            }

            int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket == -1)
            {
                PLOG_FATAL << std::string(__FUNCTION__) + ":INVALID_SOCKET ";
                continue;
            }

            bool bConnectionSuccessful = (connect(clientSocket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == 0);

            if (bConnectionSuccessful)
            {
                m_bTCPConnected = true;
                std::thread serverThread([this, &clientSocket] { RunServerThread(clientSocket); });
                serverThread.detach();
            }
            else
            {
                close(clientSocket);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void TCPRxModule::ListenForClients()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1) {
        return;
    }

    sockaddr_in listenAddr;
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(m_u16TCPPort);
    signal(SIGPIPE, SIG_IGN);

    if (inet_pton(AF_INET, m_sBindIPAddress.c_str(), &(listenAddr.sin_addr)) <= 0)
    {
        PLOG_ERROR << std::string(__FUNCTION__) + ": Invalid IP address ";
        throw;
    }

    if (bind(serverSocket, (struct sockaddr *)&listenAddr, sizeof(listenAddr)) < 0) {
        close(serverSocket);
        return;
    }

    listen(serverSocket, 1);

    while (!m_bShutDown) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            std::thread serverThread([this, &clientSocket] { RunServerThread(clientSocket); });
            serverThread.detach();
        }
    }

    close(serverSocket);
}

void TCPRxModule::RunServerThread(int &clientSocket)
{
    std::vector<char> vcAccumulatedBytes;
    vcAccumulatedBytes.reserve(2048);

    while (!m_bShutDown)
    {
        bool bSocketErrorOccured = false;

        std::vector<char> vcByteData;
        vcByteData.resize(512);
        int uReceivedDataLength = recv(clientSocket, &vcByteData[0], 512, 0);

        bSocketErrorOccured = CheckForSocketReadErrors(uReceivedDataLength, vcByteData.size());
        if (bSocketErrorOccured)
            break;

        for (int i = 0; i < uReceivedDataLength; i++)
            vcAccumulatedBytes.emplace_back(vcByteData[i]);

        if (bSocketErrorOccured)
            break;

        if (vcAccumulatedBytes.size() < 2)
            continue; // Not enough data for size header

        uint16_t u16SessionTransmissionSize;
        memcpy(&u16SessionTransmissionSize, &vcAccumulatedBytes[0], 2);

        if (vcAccumulatedBytes.size() < 2 + u16SessionTransmissionSize)
            continue; // Not enough data for size header

        auto vcCompleteClassByteVector = std::vector<char>(
            vcAccumulatedBytes.begin() + 2,
            vcAccumulatedBytes.begin() + 2 + u16SessionTransmissionSize);

        auto vcReducedAccumulatedBytes = std::vector<char>(
            vcAccumulatedBytes.begin() + u16SessionTransmissionSize,
            vcAccumulatedBytes.end());

        vcAccumulatedBytes = vcReducedAccumulatedBytes;

        auto pUDPDataChunk = std::make_shared<ByteChunk>(m_iDatagramSize);
        pUDPDataChunk->m_vcDataChunk = vcCompleteClassByteVector;

        TryPassChunk(std::dynamic_pointer_cast<BaseChunk>(pUDPDataChunk));
    }

    close(clientSocket);
    m_bTCPConnected = false;
}

bool TCPRxModule::CheckForSocketReadErrors(ssize_t stReportedSocketDataLength, size_t stActualDataLength)
{
    // Check for timeout
    if (stReportedSocketDataLength == -1 && errno == EAGAIN)
    {
        std::string strWarning = std::string(__FUNCTION__) + ": recv() timed out or errored (error: " + std::to_string(errno) + ") " + m_sBindIPAddress;
        PLOG_WARNING << strWarning;
        return true;
    }
    else if (stReportedSocketDataLength == 0)
    {
        // connection closed
        std::string strInfo = std::string(__FUNCTION__) + ": client " + m_sBindIPAddress + " closed connection, shutting down thread";
        PLOG_INFO << strInfo;
        return true;
    }

    // Check if received data length is valid
    if (stReportedSocketDataLength > stActualDataLength)
    {
        std::string strWarning = std::string(__FUNCTION__) + ": Closed connection to " + std::to_string(m_u16TCPPort) + ": received data length shorter than actual received data ";
        PLOG_WARNING << strWarning;
        return true;
    }

    return false;
}

void TCPRxModule::StartProcessing()
{
    m_thread = std::thread([this] { Process(); });
} 
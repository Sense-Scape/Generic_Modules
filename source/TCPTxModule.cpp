#include "TCPTxModule.h"

TCPTxModule::TCPTxModule(std::string sIPAddress, uint16_t u16TCPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) : BaseModule(uMaxInputBufferSize),
                                                                                                                                          m_sDestinationIPAddress(sIPAddress),
                                                                                                                                          m_u16TCPPort(u16TCPPort),
                                                                                                                                          m_WinSocket(),
                                                                                                                                          m_SocketStruct(),
                                                                                                                                          m_bTCPConnected()
{
}

TCPTxModule::~TCPTxModule()
{
    close(m_WinSocket);
}

void TCPTxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Constantly looking for new connections and stating client threads
    // One thread should be created at a time, corresponding to one simulated device
    // In the case of an error, the thread will close and this will recreate the socket

    // Lets start by creating the sock addr
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(m_u16TCPPort);
    // and prevent crashes when server closes ubruptly and casues sends to fail
    signal(SIGPIPE, SIG_IGN);

    bool bPrintedOnThisReconnect = false;

    while (!m_bShutDown)
    {
        if (!m_bTCPConnected)
        {
            if(!bPrintedOnThisReconnect)
            {
                std::string strInfo = std::string(__FUNCTION__) + ": Connecting to Server at ip " + m_sDestinationIPAddress + " on port " + std::to_string(m_u16TCPPort);
                PLOG_INFO << strInfo;
                bPrintedOnThisReconnect = true;
            }

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
            }
            
            bool bConnectionSuccessful = (connect(clientSocket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == 0);

            // Then lets do a blocking call to try connect
            if (bConnectionSuccessful)
            {
                std::string strInfo = std::string(__FUNCTION__) + ": Connected to server at ip " + m_sDestinationIPAddress + " on port " + std::to_string(m_u16TCPPort);
                PLOG_INFO << strInfo;

                // And update connection state and spin of the processing thread
                m_bTCPConnected = true;
                bPrintedOnThisReconnect = false;
                std::thread clientThread([this, &clientSocket]
                                         { RunClientThread(clientSocket); });
                clientThread.detach();
            }
            else
            {   
                close(clientSocket);        
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else
        {
            // While we are already connected lets just put the thread to sleep
            bPrintedOnThisReconnect = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void TCPTxModule::RunClientThread(int &clientSocket)
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
                    PLOG_WARNING  << "Server closed connection abruptly";
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
    std::string strInfo = std::string(__FUNCTION__) + ": Closing TCP Socket at ip " + m_sDestinationIPAddress + " on port " + std::to_string(m_u16TCPPort);
    PLOG_INFO << strInfo;

    close(clientSocket);
    m_bTCPConnected = false;
}

void TCPTxModule::StartProcessing()
{
    // Passing in empty chunk that is not used
    m_thread = std::thread([this]
                           { Process(std::shared_ptr<BaseChunk>()); });
}

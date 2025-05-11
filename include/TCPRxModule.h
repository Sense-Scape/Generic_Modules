#ifndef LIN_TCP_RX_MODULE
#define LIN_TCP_RX_MODULE

#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <atomic>
#include "BaseModule.h"
#include "ByteChunk.h"

/**
 * @brief TCP Receive Module to receive data from a TCP port
 */
class TCPRxModule : public BaseModule
{
public:
    /**
     * @brief TCPRxModule constructor
     * @param[in] sIPAddress string format of host IP address to bind/connect
     * @param[in] u16TCPPort uint16_t format of port to listen on or connect to
     * @param[in] uMaxInputBufferSize number of chunks that may be stored in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     * @param[in] strMode "Connect" to specified IP or "Listen" to specified IP
     */
    TCPRxModule(std::string sIPAddress, uint16_t u16TCPPort, unsigned uMaxInputBufferSize, int iDatagramSize, std::string strMode = "Connect");

    /**
     * @brief TCPRxModule destructor
     */
    ~TCPRxModule();

    /**
     * @brief Starts the process on its own thread
     */
    void StartProcessing() override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "TCPRxModule"; };

private:
    std::string m_sBindIPAddress;      ///< string format of host IP address to bind/connect
    uint16_t m_u16TCPPort;             ///< uint16_t format of port to listen on or connect to
    int m_Socket;                      ///< TCP socket file descriptor
    struct sockaddr_in m_SocketStruct; ///< IPv4 Socket structure
    std::atomic<bool> m_bTCPConnected; ///< State variable as to whether the TCP socket is connected
    std::string m_strMode;             ///< Store the connection mode ("Connect" or "Listen")
    int m_iDatagramSize;               ///< Size of the datagram to receive

    /**
     * @brief Module process to receive data from TCP socket and pass to next module
     */
    void Process();

    /**
     * @brief Connects to a TCP server as a client
     */
    void ConnectToServer();

    /**
     * @brief Listens for incoming TCP client connections
     */
    void ListenForClients();

    /**
     * @brief Handles receiving data from a connected client socket
     * @param[in] clientSocket Reference to the client socket file descriptor
     */
    void RunServerThread(int &clientSocket);

    /**
     * @brief Checks for errors during socket read operations
     * @param[in] stReceivedDataLength Length of data received from the socket
     * @param[in] stActualDataLength Expected length of data
     * @return true if an error occurred, false otherwise
     */
    bool CheckForSocketReadErrors(ssize_t stReportedSocketDataLength, size_t stActualDataLength);
};

#endif 
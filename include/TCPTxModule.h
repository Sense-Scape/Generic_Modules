#ifndef LIN_TCP_TX_MODULE
#define LIN_TCP_TX_MODULE

/*Standard Includes*/
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <atomic>

/*Custom Includes*/
#include "BaseModule.h"
#include "ByteChunk.h"

/**
 * @brief Windows TCP Transmit Module to transmit data from a TCP port
 */
class TCPTxModule : public BaseModule
{

public:
    /**
     * @brief TCPTxModule constructor
     * @param[in] sIPAddress string format of host IP address
     * @param[in] u16TCPPort uint16_t format of port to listen on
     * @param[in] uMaxInputBufferSize number of chunks that may be stored in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     * @param[in] strMode "Connect" to specified IP or "Listen" to specified IP
     */
    TCPTxModule(std::string sIPAddress, uint16_t u16TCPPort, unsigned uMaxInputBufferSize, int iDatagramSize, std::string strMode = "Connect");
    ~TCPTxModule();

    /**
     * @brief Starts the  process on its own thread
     */
    void StartProcessing() override;

    /**
     * @brief function called to start client thread
     * @param[in] TCP Socket
     */
    void RunClientThread(int &clientSocket);

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "TCPTxModule"; };

private:
    std::string m_sDestinationIPAddress; ///< string format of host IP address
    uint16_t m_u16TCPPort;               ///< uint16_t format of port to listen on
    int m_WinSocket;                     ///< Linux socket
    struct sockaddr_in m_SocketStruct;   ///< IPv4 Socket
    std::atomic<bool> m_bTCPConnected;   ///< State variable as to whether the TCP socket is connected
    std::string m_strMode;               ///< Store the connection mode
    
    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     * @param[in] Pointer to base chunk
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk);

    // New methods for connection modes
    void ConnectToClient();
    void ListenForClients();
};

#endif

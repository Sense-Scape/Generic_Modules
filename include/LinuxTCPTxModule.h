#ifndef LIN_TCP_TX_MODULE
#define LIN_TCP_TX_MODULE

/*Standard Includes*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

/*Custom Includes*/
#include "BaseModule.h"
#include "ByteChunk.h"

/**
 * @brief Windows TCP Transmit Module to transmit data from a TCP port
 */
class LinuxTCPTxModule : public BaseModule
{

public:
    /**
     * @brief LinuxTCPTxModule constructor
     * @param[in] sIPAddress string format of host IP address
     * @param[in] sTCPPort string format of port to listen on
     * @param[in] uMaxInputBufferSize snumber of chunk that may be stores in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     */
    LinuxTCPTxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize);
    ~LinuxTCPTxModule();

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
     * @brief Calls process function only wiht no buffer checks
     */
    void ContinuouslyTryProcess() override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "LinuxTCPTxModule"; };

private:
    std::string m_sDestinationIPAddress; ///< string format of host IP address
    std::string m_sTCPPort;              ///< string format of port to listen on
    int m_WinSocket;                     ///< Linux socket
    struct sockaddr_in m_SocketStruct;   ///< IPv4 Socket
    std::atomic<bool> m_bTCPConnected;   ///< State variable as to whether the TCP socket is connected

    /**
     * @brief Creates the windows socket using member variables
     */
    void ConnectTCPSocket();

    /*
     * @brief Closes Windows socket
     * @param[in] TCP Socket
     */
    void CloseTCPSocket(int &clientSocket);

    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     * @param[in] Pointer to base chunk
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif
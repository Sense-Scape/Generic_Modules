#ifndef LINUX_MUlTI_CLIENT_TCP_TX_MODULE
#define LINUX_MUlTI_CLIENT_TCP_TX_MODULE

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
class LinuxMultiClientTCPTxModule : public BaseModule
{

public:
    /**
     * @brief WinTCPTxModule constructor
     * @param[in] sIPAddress string format of host IP address
     * @param[in] sTCPPort string format of port to listen on
     * @param[in] uMaxInputBufferSize snumber of chunk that may be stores in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     */
    LinuxMultiClientTCPTxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize);
    ~LinuxMultiClientTCPTxModule();

    /**
     * @brief Starts the  process on its own thread
     */
    void StartProcessing() override;

    /**
     * @brief Calls process function only wiht no buffer checks
     */
    void ContinuouslyTryProcess() override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    std::string GetModuleType() override { return "LinuxMultiClientTCPTxModule"; };

private:
    std::string m_sDestinationIPAddress;   ///< string format of host IP address
    std::string m_sTCPAllocatorPortNumber; ///< string format of port to listen on
    std::atomic<bool> m_bTCPConnected;     ///< State variable as to whether the TCP socket is connected

    /**
     * @brief Conencts a Windows TCP socket on the specified port
     * @param[in] WinSocket reference to TCP socket which one wishes to use
     * @param[in] u16TCPPort uint16_t port number which one whishes to use
     */
    void ConnectTCPSocket(int &sock, uint16_t u16TCPPort);

    /**
     * @brief Waits for allocated port number using given socket
     * @param[in] WinSocket reference to TCP socket which server should reply on
     */
    uint16_t WaitForReturnedPortAllocation(int &WinSocket);

    /**
     * @brief Fucntion to start client thread data chunk tranmission
     * @param[in] WinSocket reference to TCP socket which one wishes to use
     * @param[in] u16AllocatedPortNumber uint16_t port number which one whishes to use
     */
    void RunClientThread(int &clientSocket, uint16_t u16AllocatedPortNumber);

    /*
     * @brief Closes Windows socket
     */
    void DisconnectTCPSocket(int &clientSocket);

    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     * @param[in] Pointer to base chunk
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif

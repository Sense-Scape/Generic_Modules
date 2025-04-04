#ifndef LINUX_MUlTI_CLIENT_TCP_RX_MODULE
#define LINUX_MUlTI_CLIENT_TCP_RX_MODULE

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
 * @brief Windows TCP Receiving Module to receive data from a TCP port
 */
class LinuxMultiClientTCPRxModule : public BaseModule
{

public:
    /**
     * @brief WinTCPRxModule constructor
     * @param[in] sIPAddress string format of host IP address
     * @param[in] u16TCPPort uint16_t format of port to listen on
     * @param[in] uMaxInputBufferSize number of chunks that may be stored in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     */
    LinuxMultiClientTCPRxModule(std::string sIPAddress, uint16_t u16TCPPort, unsigned uMaxInputBufferSize, int iDatagramSize);
    ~LinuxMultiClientTCPRxModule();

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
     * @param[out] ModuleType of processing module
     */
    std::string GetModuleType() override { return "LinuxMultiClientTCPRxModule"; };

private:
    std::string m_sIPAddress;              ///< String format of host IP address
    uint16_t m_u16TCPPort;                 ///< uint16_t format of port to listen on
    int m_iDatagramSize;                   ///< Maxmimum TCP buffer length
    uint16_t m_u16LifeTimeConnectionCount; ///< Number of TCP client connections arcoss time

    /**
     * @brief function called to start client thread
     * @param[in] u16TCPPort uint16_t port number which one whishes to use
     */
    void StartClientThread(uint16_t u16AllocatedPortNumber);

    /**
     * @brief function called to start client thread
     * @param[in] WinSocket reference to TCP socket which one wishes to use
     */
    void AllocateAndStartClientProcess(int &AllocatingServerSocket);

    /**
     * @brief Creates the windows socket using member variables
     * @param[in] WinSocket reference to TCP socket which one wishes to use
     * @param[in] u16TCPPort uint16_t port number which one whishes to use
     */
    void ConnectTCPSocket(int &socket, uint16_t u16TCPPort);

    /*
     * @brief Closes Windows socket
     * @param[in] socket reference to TCP socket which one wishes to use
     */
    void CloseTCPSocket(int socket);

    /**
     * @brief Checks for errors during socket read operations
     * @param[in] stReceivedDataLength Length of data received from the socket
     * @param[in] stActualDataLength Expected length of data
     * @return true if an error occurred, false otherwise
     */
    bool CheckForSocketReadErrors(ssize_t stReceivedDataLength, size_t stActualDataLength);

    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk);
};

#endif

#ifndef LINUX_MUlTI_CLIENT_TCP_TX_MODULE
#define LINUX_MUlTI_CLIENT_TCP_TX_MODULE

/*Standard Includes*/
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*Custom Includes*/
#include "BaseModule.h"

/**
 * @brief Windows TCP Transmit Module to transmit data from a TCP port
 */
class LinuxMultiClientTCPTxModule : public BaseModule {

public:
  /**
   * @brief WinTCPTxModule constructor
   * @param[in] uMaxInputBufferSize snumber of chunk that may be stores in input
   * buffer (unused)
   * @param[in] jsonConfig JSON configuration object
   */
  LinuxMultiClientTCPTxModule(unsigned uMaxInputBufferSize,
                              nlohmann::json_abi_v3_11_2::json jsonConfig);
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
  std::string GetModuleType() override {
    return "LinuxMultiClientTCPTxModule";
  };

private:
  const std::string
      m_sDestinationIPAddress;       ///< string format of host IP address
  const uint16_t m_u16TCPPort;       ///< uint16_t format of port to listen on
  std::atomic<bool> m_bTCPConnected; ///< State variable as to whether the TCP
                                     ///< socket is connected

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
   * @param[in] u16AllocatedPortNumber uint16_t port number which one whishes to
   * use
   */
  void RunClientThread(int &clientSocket, uint16_t u16AllocatedPortNumber);

  /*
   * @brief Closes Windows socket
   */
  void DisconnectTCPSocket(int &clientSocket);

  /**
   * @brief Checks for errors during socket read operations
   * @param[in] stReceivedDataLength Length of data received from the socket
   * @param[in] stActualDataLength Expected length of data
   * @return true if an error occurred, false otherwise
   */
  bool CheckForSocketReadErrors(ssize_t stReceivedDataLength,
                                size_t stActualDataLength);

  /*
   * @brief Module process to reveice data from TCP buffer and pass to next
   * module
   * @param[in] Pointer to base chunk
   */
  void Process(std::shared_ptr<BaseChunk> pBaseChunk);
};

#endif

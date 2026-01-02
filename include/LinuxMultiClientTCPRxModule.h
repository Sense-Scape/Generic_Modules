#ifndef LINUX_MUlTI_CLIENT_TCP_RX_MODULE
#define LINUX_MUlTI_CLIENT_TCP_RX_MODULE

/*Standard Includes*/
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*Custom Includes*/
#include "BaseModule.h"

/**
 * @brief Windows TCP Receiving Module to receive data from a TCP port
 */
class LinuxMultiClientTCPRxModule : public BaseModule {

public:
  /**
   * @brief WinTCPRxModule constructor
   * @param[in] uMaxInputBufferSize number of chunks that may be stored in input
   * buffer (unused)
   * @param[in] jsonConfig JSON configuration object
   */
  LinuxMultiClientTCPRxModule(unsigned uMaxInputBufferSize,
                              nlohmann::json_abi_v3_11_2::json jsonConfig);
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
  std::string GetModuleType() override {
    return "LinuxMultiClientTCPRxModule";
  };

private:
  const std::string m_strIPAddress; ///< String format of host IP address
  const uint16_t m_u16TCPPort;      ///< uint16_t format of port to listen on
  const int m_iDatagramSize;        ///< Maxmimum TCP buffer length
  uint16_t m_u16LifeTimeConnectionCount; ///< Number of TCP client connections
                                         ///< arcoss time

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
  bool CheckForSocketReadErrors(ssize_t stReceivedDataLength,
                                size_t stActualDataLength);

  /*
   * @brief Module process to reveice data from TCP buffer and pass to next
   * module
   */
  void Process(std::shared_ptr<BaseChunk> pBaseChunk);
};

#endif

#ifndef TCP_RX_MODULE
#define TCP_RX_MODULE

#include "BaseModule.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief TCP Receive Module to receive data from a TCP port
 */
class TCPRxModule : public BaseModule {
public:
  /**
   * @brief TCPRxModule constructor
   * @param[in] uMaxInputBufferSize number of chunks that may be stored in input
   * buffer
   * @param[in] jsonConfig JSON configuration object
   */
  TCPRxModule(unsigned uMaxInputBufferSize,
              nlohmann::json_abi_v3_11_2::json jsonConfig);

  /*
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
  const std::string
      m_strBindIPAddress; ///< string format of host IP address to bind/connect
  const uint16_t
      m_u16TCPPort; ///< uint16_t format of port to listen on or connect to
  const std::string
      m_strMode; ///< Store the connection mode ("Connect" or "Listen")
  const uint32_t m_u32DatagramSize;  ///< Size of the datagram to receive
  int m_Socket;                      ///< TCP socket file descriptor
  struct sockaddr_in m_SocketStruct; ///< IPv4 Socket structure
  std::atomic<bool> m_bTCPConnected; ///< State variable as to whether the TCP
                                     ///< socket is connected

  /**
   * @brief Module process to receive data from TCP socket and pass to next
   * module
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
  bool CheckForSocketReadErrors(ssize_t stReportedSocketDataLength,
                                size_t stActualDataLength);
};

#endif

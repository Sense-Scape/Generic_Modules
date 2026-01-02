#ifndef LIN_TCP_TX_MODULE
#define LIN_TCP_TX_MODULE

/*Standard Includes*/
#include <arpa/inet.h>
#include <atomic>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*Custom Includes*/
#include "BaseModule.h"

/**
 * @brief Windows TCP Transmit Module to transmit data from a TCP port
 */
class TCPTxModule : public BaseModule {

public:
  /**
   * @brief TCPTxModule constructor
   * @param[in] uMaxInputBufferSize number of chunks that may be stored in input
   * @param[in] jsonConfig JSON configuration object
   */
  TCPTxModule(unsigned uMaxInputBufferSize,
              nlohmann::json_abi_v3_11_2::json jsonConfig);
  //~TCPTxModule();

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
  const std::string
      m_sDestinationIPAddress;       ///< string format of host IP address
  const uint16_t m_u16TCPPort;       ///< uint16_t format of port to listen on
  const std::string m_strMode;       ///< Store the connection mode
  struct sockaddr_in m_SocketStruct; ///< IPv4 Socket
  std::atomic<bool> m_bTCPConnected; ///< State variable as to whether the TCP
                                     ///< socket is connected
  int m_Socket;                      ///< Linux socket

  /*
   * @brief Module process to reveice data from TCP buffer and pass to next
   * module
   * @param[in] Pointer to base chunk
   */
  void Process(std::shared_ptr<BaseChunk> pBaseChunk);

  /*
   * @brief Attempts to connect to client and then will transmit data to it
   */
  void ConnectToClient();

  /*
   * @brief Waits for a client to connect and then transimties data to it
   */
  void ListenForClients();
};

#endif

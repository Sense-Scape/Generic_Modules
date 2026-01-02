#include "TCPTxModule.h"
#include <cstdint>

#include "ByteChunk.h"
#include <signal.h>

TCPTxModule::TCPTxModule(unsigned uMaxInputBufferSize,
                         nlohmann::json_abi_v3_11_2::json jsonConfig)
    : BaseModule(uMaxInputBufferSize),
      m_sDestinationIPAddress(CheckAndThrowJSON<std::string>(jsonConfig, "IP")),
      m_u16TCPPort(CheckAndThrowJSON<uint16_t>(jsonConfig, "Port")),
      m_SocketStruct(), m_bTCPConnected(false),
      m_strMode(CheckAndThrowJSON<std::string>(jsonConfig, "Mode")) {}

void TCPTxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk) {
  // Call the appropriate connection method based on the mode
  if (m_strMode == std::string("Connect")) {
    ConnectToClient();
  } else if (m_strMode == std::string("Listen")) {
    ListenForClients();
  } else {
    PLOG_ERROR << std::string(__FUNCTION__) + ": Invalid Connection Mode";
    throw;
  }
}

void TCPTxModule::ConnectToClient() {
  // Constantly looking for new connections and stating client threads
  // One thread should be created at a time, corresponding to one simulated
  // device In the case of an error, the thread will close and this will
  // recreate the socket

  // Lets start by creating the sock addr
  sockaddr_in sockaddr;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(m_u16TCPPort);
  // and prevent crashes when server closes ubruptly and casues sends to fail
  signal(SIGPIPE, SIG_IGN);

  bool bPrintedOnThisReconnect = false;

  while (!m_bShutDown) {
    if (!m_bTCPConnected) {
      if (!bPrintedOnThisReconnect) {
        std::string strInfo = std::string(__FUNCTION__) +
                              ": Connecting to Server at ip " +
                              m_sDestinationIPAddress + " on port " +
                              std::to_string(m_u16TCPPort);
        PLOG_INFO << strInfo;
        bPrintedOnThisReconnect = true;
      }

      // Lets then convert an IPv4 or IPv6 to its binary representation
      if (inet_pton(AF_INET, m_sDestinationIPAddress.c_str(),
                    &(sockaddr.sin_addr)) <= 0) {
        std::string strWarning =
            std::string(__FUNCTION__) + ": Invalid IP address ";
        PLOG_WARNING << strWarning;
        return;
      }

      auto clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (clientSocket == -1) {
        std::string strFatal = std::string(__FUNCTION__) + ":INVALID_SOCKET ";
        PLOG_FATAL << strFatal;
      }
      // Add SO_REUSEADDR option
      int yes = 1;
      setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

      bool bConnectionSuccessful =
          (connect(clientSocket, (struct sockaddr *)&sockaddr,
                   sizeof(sockaddr)) == 0);

      // Then lets do a blocking call to try connect
      if (bConnectionSuccessful) {
        std::string strInfo = std::string(__FUNCTION__) +
                              ": Connected to server at ip " +
                              m_sDestinationIPAddress + " on port " +
                              std::to_string(m_u16TCPPort);
        PLOG_INFO << strInfo;

        // And update connection state and spin of the processing thread
        m_bTCPConnected = true;
        bPrintedOnThisReconnect = false;
        std::thread clientThread(
            [this, &clientSocket] { RunClientThread(clientSocket); });
        clientThread.detach();
      } else {
        close(clientSocket);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    } else {
      // While we are already connected lets just put the thread to sleep
      bPrintedOnThisReconnect = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

void TCPTxModule::ListenForClients() {
  int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (serverSocket == -1) {
    // Handle socket creation error
    return;
  }
  // Add SO_REUSEADDR option
  int yes = 1;
  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in listenAddr;
  listenAddr.sin_family = AF_INET;
  listenAddr.sin_port = htons(m_u16TCPPort);

  // and prevent crashes when server closes ubruptly and casues sends to fail
  signal(SIGPIPE, SIG_IGN);

  if (inet_pton(AF_INET, m_sDestinationIPAddress.c_str(),
                &(listenAddr.sin_addr)) <= 0) {
    std::string strError = std::string(__FUNCTION__) + ": Invalid IP address ";
    PLOG_ERROR << strError;
    throw;
  }

  PLOG_INFO << std::string(__FUNCTION__) + ": Listening for connections on " +
                   m_sDestinationIPAddress + " on port " +
                   std::to_string(m_u16TCPPort);

  if (bind(serverSocket, (struct sockaddr *)&listenAddr, sizeof(listenAddr)) <
      0) {
    // Handle bind error
    PLOG_INFO << std::string(__FUNCTION__) + ": Unable to bind on " +
                     m_sDestinationIPAddress + " on port " +
                     std::to_string(m_u16TCPPort) + " - " +
                     std::strerror(errno);
    close(serverSocket);
    return;
  }

  listen(serverSocket, 1); // Listen for incoming connections

  while (!m_bShutDown) {
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket >= 0) {
      // Handle new client connection
      std::thread clientThread(
          [this, &clientSocket] { RunClientThread(clientSocket); });
      clientThread.detach();
    }
  }

  close(serverSocket); // Close the listening socket when done
}

void TCPTxModule::RunClientThread(int &clientSocket) {
  while (!m_bShutDown) {
    try {
      // During processing we see if there is data (UDP Chunk) in the input
      // buffer
      std::shared_ptr<BaseChunk> pBaseChunk;
      if (TakeFromBuffer(pBaseChunk)) {
        // Cast it back to a UDP chunk
        auto udpChunk = std::static_pointer_cast<ByteChunk>(pBaseChunk);
        const auto pvcData = udpChunk->m_vcDataChunk;
        size_t length = pvcData.size();

        // And then transmit (wohoo!!!)
        int bytes_sent = send(clientSocket, &pvcData[0], length, 0);
        if (bytes_sent < 0) {
          PLOG_WARNING << "Server closed connection abruptly";
          break;
        }
      } else {
        // Wait to be notified that there is data available
        std::unique_lock<std::mutex> BufferAccessLock(m_BufferStateMutex);
        m_cvDataInBuffer.wait(BufferAccessLock, [this] {
          return (!m_cbBaseChunkBuffer.empty() || m_bShutDown);
        });
      }
    } catch (const std::exception &e) {
      std::cout << e.what() << std::endl;
      break;
    }
  }

  // In the case of stopping processing or an error we
  // formally close the socket and update state variable
  std::string strInfo =
      std::string(__FUNCTION__) + ": Closing TCP Socket at ip " +
      m_sDestinationIPAddress + " on port " + std::to_string(m_u16TCPPort);
  PLOG_INFO << strInfo;

  close(clientSocket);
  m_bTCPConnected = false;
}

void TCPTxModule::StartProcessing() {
  // Passing in empty chunk that is not used
  m_thread = std::thread([this] { Process(std::shared_ptr<BaseChunk>()); });
}

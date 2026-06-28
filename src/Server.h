#include <set>
#include <string>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <vector>

#include "types.h"

namespace Server {

constexpr int MaxEvents = 128;

struct Client {
  Client(const int fd) : fd(fd) {}
  Client() = default;

  int fd = -1;
  std::string name;
  std::string readBuffer;
  std::string writeBuffer;
  std::set<std::string> rooms;
  bool greeted = false;
};

//NOTE: if i need to resize eve
class Server {
 public:
  Server(std::string&& host = "127.0.0.1", u64 port=12345);
  ~Server();

  Server(const Server&) = delete;
  Server(const Server&&) = delete;
  auto operator=(const Server&) -> Server& = delete;
  auto operator=(const Server&&) -> Server& = delete;

  void run();

 private:
  std::string host_;
  u64 port_;
  i32 listen_fd_ = -1;
  i32 epoll_fd_ = -1;

  void setupListener();

  void run_event_loop();
  void accept_client(i32 fd);
  void handle_read(i32 fd);
  void handle_write(i32 fd);
  void close_conn(i32 fd);
};

}  // namespace Server

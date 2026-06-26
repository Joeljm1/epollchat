#include <set>
#include <string>

#include "types.h"

namespace Server {

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

class Server {
 public:
  Server(std::string&& host, u64 port);
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
};

}  // namespace Server

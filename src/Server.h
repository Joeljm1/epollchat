#include <arpa/inet.h>
#include <cerrno>
#include <deque>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "types.h"

namespace Server {

constexpr int MaxEvents = 128;

struct Client {
  Client(const int fd) : fd(fd) {}

  int fd;
  std::string username;
  std::array<char, 4096> read_buff{};
  std::string read_buffer_;
  i32 currRoom = 0;
  std::deque<std::string> queue;

  void write_data(){
    //TODO: send all data (dont forget MSG_NOSIGNAL i think)  from queue
  }
};

class Room {
  std::string name_;
  std::unordered_set<int> clients_;
  std::string password_;
  int creator_fd_;

  void add_client(int fd) { clients_.emplace(fd); }

  void remove_client(int fd) { clients_.erase(fd); }


 public:
  Room(int creator_fd, std::string&& name)
      : name_(std::move(name)),
        clients_({creator_fd}),
        creator_fd_(creator_fd) {}

  Room(int creator_fd, std::string&& password, std::string&& name)
      : name_(std::move(name)),
        clients_({creator_fd}),
        password_(std::move(password)),
        creator_fd_(creator_fd) {}
  void broadcast(std::string msg) {
    //TODO: modify epoll behavious for sock for EPOLLOUT and then send to queue
  }
};

// NOTE: if i need to resize eve
class Server {
 public:
  Server(std::string&& host = "127.0.0.1", u64 port = 12345);
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

  std::unordered_map<i32, std::unique_ptr<Client>> clients_;  // fd to clients

  std::unordered_map<i32, Room> rooms = {
      {0, Room(-1, "default")}};  // room name ->set of clients

  void setupListener();

  void run_event_loop();
  void accept_client();
  void handle_read(i32 fd);
  void handle_write(i32 fd);
  void close_conn(i32 fd);
};

}  // namespace Server

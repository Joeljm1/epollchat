#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "types.h"

#define Err(x) \
  std::cerr << (x) << "FILE: " << __FILE__ << ", LINE: " << __LINE__ << "\n"

namespace Server {

constexpr int MaxEvents = 128;

struct Client {
  Client(const int fd) : fd(fd) {}

  int fd;
  std::string username = "(Anonynous)";
  std::array<char, 4096> read_buff{};
  i32 currRoom = 0;
  std::queue<std::string> queue;

  void write_data() {
    // TODO: send all data (dont forget MSG_NOSIGNAL i think)  from queue
  }
};

class Server;
class Room {
  std::string name_;
  std::unordered_set<int> clients_;
  std::string password_;
  int creator_fd_;

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

  // TODO: some other way for the passing of map
  void broadcast(
      std::string msg,
      const std::unordered_map<i32, std::unique_ptr<Client>>& clients_map,
      i32 epoll_fd) {
    // TODO: modify epoll behaviour for sock for EPOLLOUT and then send to queue
    for (int client_id : clients_) {
      auto client_it = clients_map.find(client_id);
      if (client_it == clients_map.end()) {
        clients_.erase(client_id);
        continue;
      }
      epoll_event event{.events = EPOLLIN | EPOLLOUT | EPOLLET};
      event.data.fd = client_id;

      Client* client = client_it->second.get();
      client->queue.push(std::move(msg));
      if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_id, &event) < 0) {
        std::cerr << "epoll send msg to client failed\n";
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_id, nullptr);
        close(client_id);
      }
    }
  }
  void add_client(int fd) { clients_.emplace(fd); }

  void remove_client(int fd) { clients_.erase(fd); }
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
  const std::unordered_map<i32, std::unique_ptr<Client>>& get_client_map()
      const {
    return clients_;
  }
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

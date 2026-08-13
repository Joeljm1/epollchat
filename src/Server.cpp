
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <memory>

#include "Server.h"

namespace Server {

Server::Server(std::string&& host, u64 port)
    : port_(port), host_(std::move(host)) {}

Server::~Server() {
  // TODO:Add cleanup logic to close listen_fd_ and epoll_fd_
}

void Server::run() {
  signal(SIGPIPE, SIG_IGN);
  setupListener();
  run_event_loop();
  // TODO:
}
void Server::setupListener() {
  struct addrinfo hints{};
  struct addrinfo* res = nullptr;
  struct addrinfo* p = nullptr;

  std::memset(&hints, 0, sizeof hints);

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  std::string port_str = std::to_string(port_);
  const char* host = host_.empty() ? nullptr : host_.c_str();
  int rv = getaddrinfo(nullptr, port_str.c_str(), &hints, &res);
  if (rv != 0) {
    throw std::runtime_error(std::string("Address resolution failed: ") +
                             gai_strerror(rv));
  }

  for (p = res; p != nullptr; p = p->ai_next) {
    listen_fd_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listen_fd_ < 0) {
      continue;
    }
    int yes = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    int bindResp = bind(listen_fd_, p->ai_addr, p->ai_addrlen);
    if (bindResp < 0) {
      close(listen_fd_);
      continue;
    }
    break;
  }

  if (res != nullptr) {
    freeaddrinfo(res);
  }

  if (p == nullptr) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to bind to socket");
  }

  fcntl(listen_fd_, F_SETFL, O_NONBLOCK);
  int listResp = listen(listen_fd_, SOMAXCONN);
  if (listResp == -1) {
    throw std::system_error(errno, std::generic_category(), "Listen error");
  }

  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to create epoll instance");
  }
  epoll_event event{};
  event.events = EPOLLIN;
  event.data.fd = listen_fd_;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &event) < 0) {
    throw std::system_error(errno, std::generic_category(),
                            "epoll_ctl listen add failed");
  }
}

void Server::run_event_loop() {
  std::vector<epoll_event> events(MaxEvents);
  while (true) {
    const int ready = epoll_wait(epoll_fd_, events.data(),
                                 static_cast<int>(events.size()), -1);
    if (ready < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "epoll_wait failed");
    }
    for (int i = 0; i < ready; i++) {
      const i32 fd = events[i].data.fd;
      const u32 flags = events[i].events;
      if (fd == listen_fd_) {
        accept_client();
        // Check for errors or hang-ups or closed
      } else if ((flags & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) != 0) {
        std::cout << "Client disconnected (HUP/ERR)\n";
        close_conn(fd);
      } else if ((flags & EPOLLIN) != 0) {
        handle_read(fd);
      } else if (clients_.contains(fd) && (flags & EPOLLOUT) != 0) {
        handle_write(fd);
      } else {
        Err("Unknown epoll flag");
      }
    }
  }
}

void Server::accept_client() {
  struct sockaddr_storage storage{};
  socklen_t sock_len = sizeof storage;
  while (true) {
    int client_fd = accept4(listen_fd_, reinterpret_cast<sockaddr*>(&storage),
                            &sock_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }

      std::cerr << "accept failed: " << std::strerror(errno) << '\n';
      return;
    }
    epoll_event event{
        .events = EPOLLIN | EPOLLRDHUP,
    };
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &event) < 0) {
      Err("epoll add client failed\n");
      epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
      close(client_fd);
      continue;
      // TODO: broadcast this to others in default room
    }
    auto client = std::make_unique<Client>(client_fd);
    clients_.emplace(client_fd, std::move(client));
  }
}

void Server::handle_write(i32 fd) {
  auto client_it = clients_.find(fd);
  if (client_it == clients_.end()) {
    return;
  }
  std::queue<std::string>& queue = client_it->second->queue;
  while (!queue.empty()) {
    std::string& top_msg = queue.front();
    ssize_t bytes = send(fd, top_msg.c_str(), top_msg.size(), MSG_NOSIGNAL);
    if (static_cast<size_t>(bytes) == top_msg.size()) {  // top msg fully send
      queue.pop();
    } else if (bytes > 0) {  // top msg partially send
      top_msg.erase(0, bytes);
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;  // kerna; buffer is full i think so wait for next EPOLLOUT
    } else if (bytes < 0) {
      close_conn(fd);
      return;
    } else {
      Err("Unknown case");
      return;
    }
  }

  if (queue.empty()) {
    // TODO: verify flags
    epoll_event event{.events = EPOLLIN | EPOLLRDHUP, .data{}};
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      close_conn(fd);
    }
  }
}

// void Server::handle_read(i32 fd) {
//   auto it = clients_.find(fd);
//   if (it == clients_.end()) {
//     return;
//   }
//   bool disconnect = false;
//   it->second->read_buffer_.append(it->second->username);
//   while (true) {
//     const ssize_t bytes =
//         recv(fd, it->second->read_buff.data(), it->second->read_buff.size(),
//         0);
//     if (bytes == 0) {
//       disconnect = true;
//       break;
//     }
//     if (bytes > 0) {
//       it->second->read_buffer_.append(it->second->read_buff.data(), bytes);
//     } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
//       break;
//     } else {
//       disconnect = true;
//     }
//   }
//   if (disconnect) {
//     close_conn(fd);
//     return;
//   }
//   i32 roomId = it->second->currRoom;
//   auto room = rooms.find(roomId);
//   if (room != rooms.end()) {
//     room->second.broadcast(it->second->read_buffer_.c_str(), size);
//   }
// }

// handle_read reads the msg from kernal buffer for epoll and broadcasts the msg
// here each line (ie till '\n') is broadcasted seperatly as multiple msgs could
// be held togeather by the kernal
void Server::handle_read(i32 fd) {
  auto it = clients_.find(fd);
  if (it == clients_.end()) {
    Err("Client does not exist");
    return;
  }
  // Client& client = *it->second; not sure  if i want to do this instead
  Client* client = it->second.get();
  auto& buff = client->read_buff;
  std::string write_buff;
  while (true) {
    i64 bytes = recv(fd, buff.data(), buff.size(), 0);
    if (bytes == 0) {
      close_conn(fd);
      return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }
    if (bytes > 0) {
      write_buff.append(buff.data(), bytes);
    } else {
      close_conn(fd);
      return;
    }
  }
  u64 pos = 0;
  i32 room_id = client->currRoom;
  auto room_it = rooms.find(room_id);
  if (room_it == rooms.end()) {
    return;
  }
  Room& room = room_it->second;
  std::string_view view = write_buff;
  std::string msg;
  std::string broadcast_msg = client->username + " : ";
  while ((pos = view.find('\n')) != std::string::npos) {
    auto sub_str = view.substr(0, pos);
    if (!sub_str.empty() && sub_str.back() == '\r') {
      sub_str.remove_suffix(1);
    }
    msg.clear();
    msg.append(broadcast_msg);
    msg.append(sub_str);
    msg.push_back('\n');
    room.broadcast(msg, get_client_map(), epoll_fd_);
    view.remove_prefix(pos + 1);
  }
}

void Server::close_conn(i32 fd) {
  auto it = clients_.find(fd);
  if (it == clients_.end()) {
    return;
  }
  for (auto& [_, room] : rooms) {
    room.remove_client(fd);
  }
  clients_.erase(it);
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
  close(fd);
}

}  // namespace Server

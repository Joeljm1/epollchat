
#include "Server.h"

namespace Server {


Server::Server(std::string&& host, u64 port)
    : port_(port), host_(std::move(host)) {}

Server::~Server() {
  // TODO:
}

void Server::run() {
  signal(SIGPIPE, SIG_IGN);
  setupListener();
  // TODO:
}
void Server::setupListener() {
  struct sockaddr_storage storage{};
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
    const int ready = epoll_wait(epoll_fd_, events.data(), MaxEvents, -1);
    if (ready < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "epoll_wait failed");
    }
    for (int i = 0; i < ready; i++) {
      const i32 fd = events[i].data.fd;
      const u32 flags = events[i].events;
      if (fd == listen_fd_) {
        accept_client(fd);
        // Check for errors or hang-ups or closed
      } else if ((flags & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) != 0) {
        std::cout << "Client disconnected (HUP/ERR)\n";
        close_conn(fd);
      } else if ((flags & EPOLLIN) != 0) {
        handle_read(fd);
      } else if ( true/* check if client fd*/ &&(flags & EPOLLOUT) != 0) {
        handle_write(fd);
      } else {
        std::cerr << "Unknown epoll flag\n";
      }
    }
  }
}

}  // namespace Server

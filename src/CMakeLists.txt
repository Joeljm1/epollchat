add_executable(epoll_chat
  main.cpp
  Server.cpp
)
target_compile_features(epoll_chat PUBLIC cxx_std_23)
target_compile_options(epoll_chat PUBLIC -Wall -Wextra -Wpedantic)


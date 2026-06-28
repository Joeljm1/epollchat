add_executable(compiler
  main.cpp
  Server.cpp
)
target_compile_features(compiler PUBLIC cxx_std_23)
target_compile_options(compiler PUBLIC -Wall -Wextra -Wpedantic)


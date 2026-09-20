#include <stdexcept>
#include <system_error>

#include "Server.h"

int main() {
  Server::Server server("127.0.0.1", 12345);
  try {
    server.run();
  } catch (std::runtime_error& err) {
  } catch (std::system_error& err) {
  }
}

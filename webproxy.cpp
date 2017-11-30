#include "debug.h"
#include "netutils.h"
#include "utils.h"
#include <iostream>
#include <thread>
int cache_timeout;
int main(int argc, char** argv)
{
  u_short listen_fd, port, conn_fd, timeout;
  struct sockaddr_in remote_addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  if (argc < 3 || argc > 3) {
    Utils::print_error("USAGE: <PORT> <TIMEOUT>");
    exit(EXIT_FAILURE);
  }
  std::string file_path = "BLOCKED";
  NetUtils::fill_block_ip(file_path);
  port = (u_short)strtoul(argv[1], NULL, 0);
  timeout = (u_short)strtoul(argv[2], NULL, 0);
  listen_fd = NetUtils::create_socket(port);
  Utils::set_cache_timeout(timeout);
  while (true) {
    debug("Waiting to accept connection");
    if ((conn_fd = accept(listen_fd, (struct sockaddr*)&remote_addr, &addr_size)) < 0) {
      Utils::print_error_with_message("Error Accepting Connection");
    }
    debug("Accepted Connection");
    std::thread t(&NetUtils::spawn_request_handler, conn_fd);
    t.detach();
  }
  return 0;
}

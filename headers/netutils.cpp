#include "netutils.hpp"

u_short NetUtils::create_socket(u_short port)
{
  u_short sockfd;
  int yes = 1;
  struct sockaddr_in sin;

  memset(&sin, 0, sizeof(struct sockaddr_in));

  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = INADDR_ANY;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Utils::print_error_with_message("Unable to create socket");
    exit(EXIT_FAILURE);
  }

  if (bind(sockfd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) < 0) {
    Utils::print_error_with_message("Unable to bind socket");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    Utils::print_error_with_message("Unable to set so_reuseaddr");
    exit(EXIT_FAILURE);
  }
  if (listen(sockfd, NetUtils::MAX_CONNECTION) < 0) {
    Utils::print_error_with_message("Unable to call listen on socket");
    exit(EXIT_FAILURE);
  }
  return sockfd;
}

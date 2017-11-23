#include "netutils.hpp"

namespace NetUtils {
std::mutex netUtilsMutex;
std::unordered_map<std::string, std::string> dns_cache;
};

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

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    Utils::print_error_with_message("Unable to set so_reuseaddr");
    exit(EXIT_FAILURE);
  }
  if (bind(sockfd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) < 0) {
    Utils::print_error_with_message("Unable to bind socket");
    exit(EXIT_FAILURE);
  }

  if (listen(sockfd, NetUtils::MAX_CONNECTION) < 0) {
    Utils::print_error_with_message("Unable to call listen on socket");
    exit(EXIT_FAILURE);
  }
  return sockfd;
}

std::string NetUtils::resolve_host_name(std::string hostname)
{
  std::string ip;
  std::lock_guard<std::mutex> guard(NetUtils::netUtilsMutex);
  if (NetUtils::dns_cache.find(hostname) != NetUtils::dns_cache.end()) {
    debug("Hostname found in Cache");
    ip = NetUtils::dns_cache[hostname];
  } else {
    debug("Hostname NOT found in cache");
    struct hostent* lh = gethostbyname(hostname.c_str());
    if (lh) {
      struct in_addr** addr_list;
      addr_list = (struct in_addr**)lh->h_addr_list;
      ip = std::string(inet_ntoa(*addr_list[0]));
    } else {
      herror("Unable to resolve address");
      Utils::print_error_with_message("Unable to resolve address");
    }
    NetUtils::dns_cache[hostname] = ip;
  }
  return ip;
}

void NetUtils::spawn_request_handler(u_short socket)
{
  int status;
  NetUtils::Request rq = NetUtils::Request(socket);
  while (true) {
    debugs("Thread ID", std::this_thread::get_id());
    status = rq.readRequestFromSocket();
    if (status == NetUtils::Request::CONNECTION_CLOSED) {
      debug("Connection Closed");
      break;
    }
    debugs("Request Received", rq);
  }
}

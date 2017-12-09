#include "debug.h"
#include "http.h"
#include "utils.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#pragma once
namespace NetUtils {

extern std::set<std::string> blocked_ip;
extern std::set<std::string> blocked_host;
extern std::mutex netUtilsMutex;
extern std::unordered_map<std::string, std::string> dns_cache;

extern int MAX_CONNECTION;
extern int DEFAULT_PORT;
extern int DEFAULT_BUFFLEN;
extern int ERROR;
extern int SUCCESS;
extern int CONNECTION_CLOSED;

u_short create_socket(u_short);
void spawn_request_handler(u_short);
void spawn_prefetch_handler(std::string, std::string, u_short, std::set<std::string>);
std::string resolve_host_name(std::string);
u_short create_remote_socket(std::string, u_short);
int send_to_socket(u_short, const void*, ssize_t, std::string);
int recv_from_socket(u_short, void*, ssize_t, std::string);
void fill_block_ip(std::string&);
void recv_headers(u_short, std::string&, std::vector<u_char>&);
void add_close_to_headers(std::string&);
class RqRsHandler {

  void setMethodUrlHttp(std::string);
  void setHostAndPort(std::string);

  public:
  static const int CONNECTION_CLOSED;
  static const int ERROR;
  static const int SUCCESS;
  static const int NO_CONTENT_LENGTH;
  static const int HOST_BLOCKED;
  static const int IP_BLOCKED;
  static const int UNKNOWN_REQUEST;
  static const int NO_HOST_RESOLVE;
  static const int RETURN_CACHE;
  std::string method;
  u_short port;
  u_short client_socket;
  u_short remote_socket;
  std::string url;
  std::string host;
  std::string payload;
  std::string http;
  std::string hostIp;
  std::string request;
  std::string response;
  std::string url_hash;
  bool host_blocked;
  bool ip_blocked;
  RqRsHandler(u_short);
  ~RqRsHandler();

  int readRqFromClient();
  int sendRqToRemote();
  int sendRsToClient();
  int sendCacheToClient();
  bool handleError(int);

  static int getContentLengthFromHeader(std::string&);
};
std::ostream& operator<<(std::ostream&, const NetUtils::RqRsHandler&);
}

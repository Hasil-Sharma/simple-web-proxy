#include "debug.h"
#include "http.h"
#include "utils.h"
#include <arpa/inet.h>
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

u_short create_socket(u_short);
void spawn_request_handler(u_short);
std::string resolve_host_name(std::string);
u_short create_remote_socket(std::string, u_short);
int send_to_socket(u_short, void*, ssize_t, std::string);
int recv_from_socket(u_short, void*, ssize_t, std::string);
void fill_block_ip(std::string&);

class RequestResponseHandler {

  void setMethodUrlHttp(std::string);
  void setHostAndPort(std::string);
  int getContentLengthFromHeader(std::string&);

  public:
  static const int CONNECTION_CLOSED = 0;
  static const int ERROR = 1;
  static const int SUCCESS = 2;
  static const int NO_CONTENT_LENGTH = -1;
  static const int HOST_BLOCKED = 3;
  static const int IP_BLOCKED = 4;
  static const int UNKNOWN_REQUEST = 5;
  static const int NO_HOST_RESOLVE = 6;
  std::string method;
  u_short port;
  u_short socket;
  u_short remote_socket;
  std::string url;
  std::string host;
  std::string payload;
  std::string http;
  std::string hostIp;
  std::string request;
  std::string response;
  bool hostBlocked;
  bool ipBlocked;
  RequestResponseHandler(u_short socket);
  ~RequestResponseHandler();

  int readRequestFromSocket();
  int readResponseFromRemote();
  int sendResponseToSocket();
  bool handleError(int);
};
std::ostream& operator<<(std::ostream&, const NetUtils::RequestResponseHandler&);
}

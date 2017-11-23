#include "debug.h"
#include "utils.h"
#include <arpa/inet.h>
#include <cstring>
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

enum netconstants {
  MAX_CONNECTION = 10,
  DEFAULT_PORT = 80,
  DEFAULT_BUFFLEN = 512,
  ERROR = -1
};

u_short create_socket(u_short);
void spawn_request_handler(u_short);
std::string resolve_host_name(std::string);
u_short create_remote_socket(std::string, u_short);
class RequestResponseHandler {

  const std::string endOfRequest = "\r\n\r\n";
  const std::string delimReq = "\r\n";
  const std::string hostProperty = "Host:";
  const std::string methodProperty = "GET";

  void setMethodUrlHttp(std::string);
  void setHostAndPort(std::string);

  public:
  static const int CONNECTION_CLOSED = 0;
  static const int ERROR = 1;
  static const int SUCCESS = 2;

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
  RequestResponseHandler(u_short socket);
  ~RequestResponseHandler();

  int readRequestFromSocket();
  int readResponseFromRemote();
  int sendResponseToSocket();
};
std::ostream& operator<<(std::ostream&, const NetUtils::RequestResponseHandler&);
}

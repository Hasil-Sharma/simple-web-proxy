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
  DEFAULT_BUFFLEN = 512
};

u_short create_socket(u_short);
void spawn_request_handler(u_short);
class RequestResponseHandler {

  private:
  void setMethodUrlHttp(std::string);
  void setHostAndPort(std::string);
  std::string endOfRequest = "\r\n\r\n";
  std::string delimReq = "\r\n";
  std::string hostProperty = "Host:";
  std::string methodProperty = "GET";

  public:
  static const int CONNECTION_CLOSED = 0;
  static const int ERROR = 1;
  static const int REQUEST_SERVED = 2;
  std::string method;
  u_short port;
  u_short socket;
  std::string url;
  std::string host;
  std::string payload;
  std::string http;
  std::string hostIp;
  RequestResponseHandler(u_short socket)
  {
    this->socket = socket;
    this->port = NetUtils::DEFAULT_PORT;
  }
  ~RequestResponseHandler()
  {
    // Closing the socket
    debug("Calling close on socket");
    close(this->socket);
  }
  int readRequestFromSocket();
};
std::ostream& operator<<(std::ostream&, const NetUtils::RequestResponseHandler&);
std::string resolve_host_name(std::string);
}

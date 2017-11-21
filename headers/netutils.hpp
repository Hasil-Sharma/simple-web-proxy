#include "debug.hpp"
#include "utils.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#ifndef NETUTILS_H
#define NETUTILS_H

namespace NetUtils {
enum netconstants {
  MAX_CONNECTION = 10,
  DEFAULT_PORT = 80,
  DEFAULT_BUFFLEN = 512
};

u_short create_socket(u_short);

class Request {
  private:
  void setMethodUrlHttp(std::string);
  void setHostAndPort(std::string);
  std::string endOfRequest = "\r\n\r\n";
  std::string delimReq = "\r\n";
  std::string hostProperty = "Host:";
  std::string methodProperty = "GET";

  public:
  std::string method;
  u_short port;
  u_short socket;
  std::string url;
  std::string host;
  std::string payload;
  std::string http;
  Request(u_short socket)
  {
    this->socket = socket;
    this->port = NetUtils::DEFAULT_PORT;
  }
  ~Request()
  {
    // Closing the socket
    debug("Calling close on socket");
    close(this->socket);
  }
  void readRequestFromSocket();
};

std::ostream& operator<<(std::ostream&, const NetUtils::Request&);
}
#endif

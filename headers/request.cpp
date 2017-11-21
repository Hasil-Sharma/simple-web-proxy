#include "netutils.hpp"

void NetUtils::Request::readRequestFromSocket()
{
  // Assumption is withing DEFAULT_BUFFLEN entire request can be read
  std::string request;
  ssize_t r_bytes = 0;
  char recvbuff[DEFAULT_BUFFLEN + 1], cbuff;
  int delimIdx = 0;
  memset(recvbuff, 0, (DEFAULT_BUFFLEN + 1) * sizeof(char));
  while (r_bytes < DEFAULT_BUFFLEN) {
    if ((r_bytes += recv(this->socket, recvbuff + r_bytes, DEFAULT_BUFFLEN - r_bytes, 0)) < 0) {
      Utils::print_error_with_message("Unable to read request from socket");
    }

    //debugs("Read from socket", r_bytes);
    request = recvbuff;
    if (request.find(this->endOfRequest) != std::string::npos)
      break;
  }

  //debugs("Request Received\n", request);
  std::vector<std::string> rq_delim = Utils::split_string_to_vector(request, this->delimReq);
  //debugs("Request Size", r_bytes);
  //debugs("Request Vector", rq_delim);

  int host_idx = Utils::find_string_index(rq_delim, this->hostProperty);
  if (host_idx == -1) {
  } // TODO
  this->setHostAndPort(rq_delim[host_idx]);
  int method_idx = Utils::find_string_index(rq_delim, this->methodProperty);
  if (method_idx == -1) {
  } // TODO
  this->setMethodUrlHttp(rq_delim[method_idx]);
}

void NetUtils::Request::setHostAndPort(std::string line)
{
  std::vector<std::string> tokens = Utils::split_string_to_vector(line, ":");
  // tokens[0] is the "Host"
  // tokens[1] is <Host IP> with a space
  // tokens[2] is PORT if specified

  this->host = tokens[1];
  if (tokens.size() > 2)
    this->port = (u_short)strtoul(tokens[2].c_str(), NULL, 0);
}
void NetUtils::Request::setMethodUrlHttp(std::string line)
{
  std::string buff;
  std::stringstream ss(line);
  std::vector<std::string> x;
  while (ss >> buff) {
    x.push_back(buff);
  }
  this->method = x[0];
  this->url = x[1];
  this->http = x[2];
  // TODO : Handle error cases here
}
std::ostream& NetUtils::operator<<(std::ostream& os, const NetUtils::Request& rq)
{
  os << "\n\tMethod: " << rq.method;
  os << "\n\tURL: " << rq.url;
  os << "\n\tHTTP: " << rq.http;
  os << "\n\tHost: " << rq.host;
  os << "\n\tPort: " << rq.port;
  return os;
}

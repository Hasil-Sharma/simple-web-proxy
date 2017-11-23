#include "netutils.h"

NetUtils::RequestResponseHandler::RequestResponseHandler(u_short socket)
{
  this->socket = socket;
  this->port = NetUtils::DEFAULT_PORT;
}

NetUtils::RequestResponseHandler::~RequestResponseHandler()
{
  // Closing the socket
  debug("Calling close on socket");
  close(this->socket);
}

int NetUtils::RequestResponseHandler::readRequestFromSocket()

{
  // Assumption is withing DEFAULT_BUFFLEN entire request can be read
  std::string request;
  ssize_t r_bytes = 0, temp_bytes;
  char recvbuff[DEFAULT_BUFFLEN + 1];
  bool connection_closed = false;
  memset(recvbuff, 0, (DEFAULT_BUFFLEN + 1) * sizeof(char));
  while (r_bytes < DEFAULT_BUFFLEN && !connection_closed) {

    if ((temp_bytes = recv(this->socket, recvbuff + r_bytes, DEFAULT_BUFFLEN - r_bytes, 0)) < 0) {
      Utils::print_error_with_message("Unable to read request from socket");
    }
    // Handling the case of closing connection
    if (temp_bytes == 0) {
      connection_closed = true;
      break;
    }
    //debugs("Read from socket", r_bytes);
    r_bytes += temp_bytes;
    request = recvbuff;
    if (request.find(this->endOfRequest) != std::string::npos)
      break;
  }

  if (connection_closed == true) {
    return CONNECTION_CLOSED;
  }
  this->request = request;
  debugs("Request Received\n", request);
  std::vector<std::string> rq_delim = Utils::split_string_to_vector(request, delimReq);
  //debugs("Request Size", r_bytes);
  //debugs("Request Vector", rq_delim);

  int host_idx = Utils::find_string_index(rq_delim, hostProperty);
  if (host_idx == -1) {
    return ERROR;
  } // TODO
  this->setHostAndPort(rq_delim[host_idx]);
  int method_idx = Utils::find_string_index(rq_delim, methodProperty);
  if (method_idx == -1) {
    return ERROR;
  } // TODO
  this->setMethodUrlHttp(rq_delim[method_idx]);
  this->hostIp = NetUtils::resolve_host_name(this->host);

  return SUCCESS;
}

int NetUtils::RequestResponseHandler ::readResponseFromRemote()
{
  u_short socket;
  ssize_t bytes_sent = 0, total_bytes = this->request.length();
  const char* request_ptr = this->request.c_str();
  this->remote_socket = NetUtils::create_remote_socket(this->hostIp, this->port);
  socket = this->remote_socket;
  if (socket == NetUtils::ERROR) {
    // TODO : Handle error cases here

    return RequestResponseHandler::ERROR;
  }

  debugs("Sending Request to remote", this->request);
  // Sending the request received from the client
  while (bytes_sent != total_bytes) {
    if ((bytes_sent += send(socket, request_ptr + bytes_sent, total_bytes - bytes_sent, 0)) < 0) {
      Utils::print_error_with_message("Unable to send request to the remote server");
      return ERROR;
    }
  }
  return SUCCESS;
}

int NetUtils::RequestResponseHandler::sendResponseToSocket()
{
  ssize_t r_bytes = 0, s_bytes, p_bytes;
  u_short remote_socket = this->remote_socket, client_socket = this->socket;
  u_char buffer[NetUtils::DEFAULT_BUFFLEN];
  struct timeval tv;
  memset(&tv, 0, sizeof(struct timeval));
  tv.tv_sec = 1;
  // This size will be the content-length
  bool client_connection_close = false, remote_connection_close = false;

  setsockopt(remote_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
  while (!client_connection_close && !remote_connection_close) {

    if ((r_bytes = recv(remote_socket, buffer, DEFAULT_BUFFLEN, 0)) < 0) {
      Utils::print_error_with_message("Unable to receive from remote server");
      return RequestResponseHandler::ERROR;
    }

    if (r_bytes == 0) {
      remote_connection_close = true;
      debug("Remote closed the connection, closing the remote socket");
      close(remote_socket);
      break;
    }

    s_bytes = 0;
    while (s_bytes != r_bytes) {
      if ((p_bytes = send(client_socket, buffer + s_bytes, r_bytes - s_bytes, 0)) < 0) {
        Utils::print_error_with_message("Unable to send response to client");
        return RequestResponseHandler::ERROR;
      }

      if (p_bytes == 0) {
        client_connection_close = true;
        break;
      }
      s_bytes += p_bytes;
    }
  }
  return RequestResponseHandler::SUCCESS;
}
void NetUtils::RequestResponseHandler::setHostAndPort(std::string line)
{
  std::vector<std::string> tokens = Utils::split_string_to_vector(line, ":");
  // tokens[0] is the "Host"
  // tokens[1] is <Host IP> with a space
  // tokens[2] is PORT if specified

  this->host = (tokens[1][0] == ' ') ? tokens[1].substr(1) : tokens[1];
  if (tokens.size() > 2)
    this->port = (u_short)strtoul(tokens[2].c_str(), NULL, 0);
}

void NetUtils::RequestResponseHandler::setMethodUrlHttp(std::string line)
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

std::ostream& NetUtils::operator<<(std::ostream& os, const NetUtils::RequestResponseHandler& rq)
{
  os << "\n\tMethod: " << rq.method;
  os << "\n\tURL: " << rq.url;
  os << "\n\tHTTP: " << rq.http;
  os << "\n\tHost: " << rq.host;
  os << "\n\tHostIp: " << rq.hostIp;
  os << "\n\tPort: " << rq.port;
  return os;
}

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
  ssize_t r_bytes, t_bytes, content_length;
  u_short remote_socket = this->remote_socket, client_socket = this->socket;
  u_char* buffer;
  char cBuff;
  std::string header;
  // Reading header line by line
  // TODO: Optimize this

  r_bytes = 0;
  while (true) {
    if ((t_bytes = recv(remote_socket, &cBuff, 1, 0)) < 0) {
      Utils::print_error_with_message("Unable to receive from remote server");
      return RequestResponseHandler::ERROR;
    }

    if (t_bytes == 0) {
      return RequestResponseHandler::CONNECTION_CLOSED;
    }
    r_bytes += t_bytes;
    header += cBuff;
    if (header.find(RequestResponseHandler::endOfRequest) != std::string::npos) {
      //debugs("Response Header Read Completely", header);
      break;
    }
  }

  content_length = this->getContentLengthFromHeader(header);

  if (content_length == NO_CONTENT_LENGTH) {
    // TODO: Handle Error
    return RequestResponseHandler::ERROR;
  }

  debug("Sending header to client");
  if (NetUtils::send_to_socket(client_socket, (void*)header.c_str(), r_bytes, "Unable to send headers to client") == NetUtils::ERROR) {
    // TODO: Handle Error
    return RequestResponseHandler::ERROR;
  }

  buffer = (u_char*)malloc(content_length * sizeof(u_char));
  memset(buffer, 0, content_length * sizeof(u_char));

  debug("Receving content from remote");
  if (NetUtils::recv_from_socket(remote_socket, (void*)buffer, content_length, "Unable to receive content from remote") == NetUtils::ERROR) {
    // TODO: Handle Error
    return RequestResponseHandler::ERROR;
  }

  debug("Sending content to client");
  if (NetUtils::send_to_socket(client_socket, (void*)buffer, content_length, "Unable to send content to client") == NetUtils::ERROR) {
    // TODO: Handle Error
    return RequestResponseHandler::ERROR;
  }

  debug("Extracting hyperlink from content");
  std::string string_buffer((char*)buffer);
  std::set<std::string> hyperlinks = Utils::extract_hyperlinks(string_buffer);

  for (std::string s : hyperlinks) {
    debugs("Hyperlink Extracted", s);
  }
  free(buffer);
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

int NetUtils::RequestResponseHandler::getContentLengthFromHeader(std::string& header)
{
  std::vector<std::string> tokens = Utils::split_string_to_vector(header, RequestResponseHandler::delimReq);
  int length = NO_CONTENT_LENGTH;
  // Search for "Content-Length:" in the header
  for (std::string s : tokens) {
    if (s.find(contentLengthProperty) != std::string::npos) {
      // Content-Length found
      std::vector<std::string> subtokens = Utils::split_string_to_vector(s, ":");
      // subTokens[1] has the content length
      std::stringstream ss(subtokens[1]);
      ss >> length;
      break;
    }
  }

  return length;
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

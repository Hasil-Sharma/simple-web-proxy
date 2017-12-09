#include "netutils.h"

const int NetUtils::RqRsHandler::CONNECTION_CLOSED = 0;
const int NetUtils::RqRsHandler::ERROR = 1;
const int NetUtils::RqRsHandler::SUCCESS = 2;
const int NetUtils::RqRsHandler::NO_CONTENT_LENGTH = 0;
const int NetUtils::RqRsHandler::HOST_BLOCKED = 3;
const int NetUtils::RqRsHandler::IP_BLOCKED = 4;
const int NetUtils::RqRsHandler::UNKNOWN_REQUEST = 5;
const int NetUtils::RqRsHandler::NO_HOST_RESOLVE = 6;
const int NetUtils::RqRsHandler::RETURN_CACHE = 6;

NetUtils::RqRsHandler::RqRsHandler(u_short client_socket)
{
  this->client_socket = client_socket;
  this->host_blocked = false;
  this->port = NetUtils::DEFAULT_PORT;
  this->remote_socket = 0;
}

NetUtils::RqRsHandler::~RqRsHandler()
{
  // Closing the socket
  debug("Calling close on socket");
  close(this->client_socket);
  if (this->remote_socket)
    close(this->remote_socket);
}

int NetUtils::RqRsHandler::readRqFromClient()
{
  // Assumption is withing DEFAULT_BUFFLEN entire request can be read
  // TODO: Fix this
  std::string request;
  std::vector<u_char> body;
  char recvbuff[DEFAULT_BUFFLEN + 1];
  bool connection_closed = false;

  memset(recvbuff, 0, (DEFAULT_BUFFLEN + 1) * sizeof(char));
  //debug("Start: Read request from client socket");
  // Headers received doesn't have the request end
  NetUtils::recv_headers(this->client_socket, request, body);
  debugs("Request from client", request);
  //debug("End: Read request from client socket");
  if (connection_closed == true) {
    return RqRsHandler::CONNECTION_CLOSED;
  }

  // Extracting parameters from the HTTP request
  std::vector<std::string> rq_delim = Utils::split_string_to_vector(request, httpconstant::fields::delim);
  request += httprequest::request_end;
  NetUtils::add_close_to_headers(request);
  this->request = request;
  int host_idx = Utils::find_string_index(rq_delim, httpconstant::fields::host);
  if (host_idx == -1) {
    return RqRsHandler::ERROR;
  } // TODO
  this->setHostAndPort(rq_delim[host_idx]);
  int method_idx = Utils::find_string_index(rq_delim, httprequest::type);
  if (method_idx == -1) {
    this->setMethodUrlHttp(rq_delim[0]);
    return RqRsHandler::UNKNOWN_REQUEST;
  }
  this->setMethodUrlHttp(rq_delim[method_idx]);
  if (NetUtils::blocked_host.find(this->host) == NetUtils::blocked_host.end())
    this->hostIp = NetUtils::resolve_host_name(this->host);
  else {
    this->host_blocked = true;
    return RqRsHandler::HOST_BLOCKED;
  }
  if (this->hostIp.length() == 0)
    return RqRsHandler::NO_HOST_RESOLVE;
  if (NetUtils::blocked_ip.find(this->hostIp) != NetUtils::blocked_ip.end()) {
    this->ip_blocked = true;
    return RqRsHandler::IP_BLOCKED;
  }

  this->url_hash = Utils::get_md5_hash(this->host + this->url);
  return RqRsHandler::SUCCESS;
}

int NetUtils::RqRsHandler::sendRqToRemote()
{
  /* Creating a new socket for Remote connection
   * Sending Connection: close with every request explicitly
   * TODO: Reuse the same socket
   */
  bool in_cache = Utils::check_in_cache(this->url_hash);
  if (in_cache) {
    debugs("Found in cache", this->url_hash);
    return RqRsHandler::RETURN_CACHE;
  }
  u_short remote_socket;
  ssize_t total_bytes = this->request.length();
  const char* request_ptr = this->request.c_str();

  // Create a new socket only if already existing socket doesn't exist
  // Update: new socket created everytime as connection is closed each time
  // TODO: Handle the case when remote closes the connection and new remote_socket is needed
  //if (this->remote_socket == 0)
  this->remote_socket = NetUtils::create_remote_socket(this->hostIp, this->port);
  remote_socket = this->remote_socket;

  if (remote_socket == NetUtils::ERROR) {
    // TODO : Handle error cases here

    return RqRsHandler::ERROR;
  }
  //debug("Start: Send Request to remote");
  // Sending the request received from the client
  // Sending Connection: close with request
  debugs("Request to remote", request);
  if (NetUtils::send_to_socket(remote_socket, request_ptr, total_bytes, "Unable to send request to the remote server") != NetUtils::SUCCESS)
    return RqRsHandler::ERROR;
  //debug("End: Send Request to remote");
  return RqRsHandler::SUCCESS;
}

int NetUtils::RqRsHandler::sendRsToClient()
{
  ssize_t content_length;
  u_short remote_socket = this->remote_socket, client_socket = this->client_socket;
  u_char* buffer;
  std::string header;
  std::vector<u_char> body;
  // Reading header line by line
  // TODO: Optimize this

  //debug("Start: Reading header from remote");
  NetUtils::recv_headers(remote_socket, header, body);
  //debug("End: Reading header from remote");
  debugs("Header from Remote", header);

  // Adding explicit close to client
  header += httprequest::request_end;
  NetUtils::add_close_to_headers(header);
  content_length = this->getContentLengthFromHeader(header);

  if (content_length == NO_CONTENT_LENGTH) {
    // TODO: Handle Error, Should close the remote socket to avoid reading the content in some other call
    //return RqRsHandler::NO_CONTENT_LENGTH;
    std::vector<u_char> buffer_vector = body;
    buffer = (u_char*)malloc(NetUtils::DEFAULT_BUFFLEN * sizeof(u_char));

    if (NetUtils::send_to_socket(client_socket, (void*)header.c_str(), header.length(), "Unable to send headers to client") == NetUtils::ERROR) {
      // TODO: Handle Error
      return RqRsHandler::ERROR;
    }

    while (true) {
      memset(buffer, 0, NetUtils::DEFAULT_BUFFLEN * sizeof(u_char));
      int status = NetUtils::recv_from_socket(remote_socket, (void*)buffer, NetUtils::DEFAULT_BUFFLEN, "Unable to receive content from remote: chunked");

      for (int i = 0; i < NetUtils::DEFAULT_BUFFLEN; i++) {
        if (buffer[i] == 0)
          break;
        buffer_vector.push_back(buffer[i]);
      }
      if (status == NetUtils::CONNECTION_CLOSED) {
        break;
      }
    }
    free(buffer);
    content_length = buffer_vector.size();
    buffer = (u_char*)malloc(content_length * sizeof(u_char));
    memset(buffer, 0, content_length * sizeof(u_char));
    std::copy(buffer_vector.begin(), buffer_vector.end(), buffer);
  } else {

    //debug("Sending header to client");
    if (NetUtils::send_to_socket(client_socket, (void*)header.c_str(), header.length(), "Unable to send headers to client") == NetUtils::ERROR) {
      // TODO: Handle Error
      return RqRsHandler::ERROR;
    }

    //if (NetUtils::send_to_socket(client_socket, (void*)&body[0], body.size(), "Unable to send partial content to client") == NetUtils::ERROR) {
    //return RqRsHandler::ERROR;
    //}
    //content_length -= body.size();
    buffer = (u_char*)malloc(content_length * sizeof(u_char));
    memset(buffer, 0, content_length * sizeof(u_char));
    for (int i = 0; i < body.size(); i++)
      buffer[i] = body[i];
    //debugs("Partial Content Recv", body.size());
    //debug("Receving content from remote");
    if (NetUtils::recv_from_socket(remote_socket, (void*)(buffer + body.size()), content_length - body.size(), "Unable to receive content from remote") == NetUtils::ERROR) {
      // TODO: Handle Error
      // close the remote socket because it shouldn't be used again
      close(remote_socket);
      this->remote_socket = 0;
      return RqRsHandler::ERROR;
    }
  }
  //debug("Sending content to client");
  if (NetUtils::send_to_socket(client_socket, (void*)buffer, content_length, "Unable to send content to client") == NetUtils::ERROR) {
    // TODO: Handle Error
    return RqRsHandler::ERROR;
  }

  // Closing the remote socket after reading the response
  //debug("Closing the Remote socket");
  close(this->remote_socket);
  this->remote_socket = 0;

  //debug("Saving the page fetched to cache");

  // Closing the client socket
  close(this->client_socket);

  // Adding explicit cache thing int cached request
  header.erase(header.find(httprequest::request_end));
  header += httpconstant::fields::delim + httpconstant::fields::cache + ": true";
  header += httprequest::request_end;

  if (header.find("200 OK") != std::string::npos) {
    Utils::save_to_cache(this->url_hash, (u_char*)header.c_str(), header.length());
    Utils::save_to_cache(this->url_hash, buffer, content_length);
    //debug("Saved the page fetched to cache");
    //debug("Extracting hyperlink from content");
    std::string string_buffer((char*)buffer);
    std::set<std::string> hyperlinks = Utils::extract_hyperlinks(string_buffer);
    std::thread(&NetUtils::spawn_prefetch_handler, this->host, this->hostIp, this->port, hyperlinks).detach();
    for (std::string s : hyperlinks) {
      debugs("Hyperlink Extracted", s);
    }
  }
  free(buffer);
  return RqRsHandler::SUCCESS;
}
void NetUtils::RqRsHandler::setHostAndPort(std::string line)
{
  std::vector<std::string> tokens = Utils::split_string_to_vector(line, ":");
  // tokens[0] is the "Host"
  // tokens[1] is <Host IP> with a space
  // tokens[2] is PORT if specified

  this->host = (tokens[1][0] == ' ') ? tokens[1].substr(1) : tokens[1];
  if (tokens.size() > 2)
    this->port = (u_short)strtoul(tokens[2].c_str(), NULL, 0);
}

void NetUtils::RqRsHandler::setMethodUrlHttp(std::string line)
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

int NetUtils::RqRsHandler::getContentLengthFromHeader(std::string& header)
{
  std::vector<std::string> tokens = Utils::split_string_to_vector(header, httpconstant ::fields ::delim);
  int length = NO_CONTENT_LENGTH;
  // Search for "Content-Length:" in the header
  for (std::string s : tokens) {
    // Content-Length should occur first as it can occur elsewhere too
    if (s.find(httpconstant::fields::content_length) == 0) {
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

int NetUtils::RqRsHandler::sendCacheToClient()
{
  std::ostringstream header;
  std::vector<u_char> body;
  Utils::read_from_cache(this->url_hash, body);
  int response_status;
  response_status = NetUtils::send_to_socket(this->client_socket, (void*)&body[0], body.size(), "Unable to send body from cache");

  return (response_status == NetUtils::SUCCESS) ? RqRsHandler::SUCCESS : RqRsHandler::ERROR;
}
bool NetUtils::RqRsHandler::handleError(int status)
{
  std::string body, response;
  std::ostringstream header;
  if (status == RqRsHandler::HOST_BLOCKED || status == RqRsHandler::IP_BLOCKED) {

    const char* template_strings[] = {
      httpresponse::templates::generic.c_str(),
      httpresponse::templates::error_403.c_str(),
      this->host.c_str()
    };

    body = Utils::generate_dynamic_string(template_strings, 3);
    header << ((this->http.compare(httpconstant::http_11) == 0) ? httpresponse::header::http_11_error_403 : httpresponse::header::http_10_error_403);
  } else if (status == RqRsHandler::UNKNOWN_REQUEST) {

    const char* template_strings[] = {
      httpresponse::templates::generic.c_str(),
      httpresponse::templates::error_400.c_str(),
      this->method.c_str()
    };

    body = Utils::generate_dynamic_string(template_strings, 3);
    header << ((this->http.compare(httpconstant::http_11) == 0) ? httpresponse::header::http_11_error_400 : httpresponse::header::http_10_error_400);
  } else if (status == RqRsHandler::NO_HOST_RESOLVE) {

    const char* template_strings[] = {
      httpresponse::templates::generic.c_str(),
      httpresponse::templates::error_404.c_str(),
      this->host.c_str()
    };

    body = Utils::generate_dynamic_string(template_strings, 3);
    header << ((this->http.compare(httpconstant::http_11) == 0) ? httpresponse::header::http_11_error_404 : httpresponse::header::http_10_error_404);
  } else if (status == RqRsHandler::NO_CONTENT_LENGTH) {
    const char* template_strings[] = {
      httpresponse::templates::generic.c_str(),
      httpresponse::templates::error_404.c_str(),
      "Content-Length not specified"
    };

    body = Utils::generate_dynamic_string(template_strings, 3);
    header << ((this->http.compare(httpconstant::http_11) == 0) ? httpresponse::header::http_11_error_404 : httpresponse::header::http_10_error_404);
  }

  header << httpconstant::fields::delim;
  header << httpconstant::fields::content_length << ": " << body.length() << httpconstant::fields::delim;
  header << httpconstant::fields::content_type << ": " << httpresponse::error_content_type << httpconstant::fields::delim;
  header << httpconstant::fields::delim;
  header << body;
  response = header.str();
  int response_status = NetUtils::send_to_socket(this->client_socket, (void*)response.c_str(), response.length(), "Unable to responsd error message");
  return (response_status == NetUtils::SUCCESS) ? true : false;
}
std::ostream& NetUtils::operator<<(std::ostream& os, const NetUtils::RqRsHandler& rq)
{
  os << "\n\tMethod: " << rq.method;
  os << "\n\tURL: " << rq.url;
  os << "\n\tHTTP: " << rq.http;
  os << "\n\tHost: " << rq.host;
  os << "\n\tHostIp: " << rq.hostIp;
  os << "\n\tPort: " << rq.port;
  return os;
}

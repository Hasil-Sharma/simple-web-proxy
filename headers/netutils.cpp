#include "netutils.h"

namespace NetUtils {
std::mutex netUtilsMutex;
std::unordered_map<std::string, std::string> dns_cache;
std::set<std::string> blocked_ip;
std::set<std::string> blocked_host;

int MAX_CONNECTION = 10;
int DEFAULT_PORT = 80;
int DEFAULT_BUFFLEN = 1024;
int ERROR = -1;
int SUCCESS = 1;
int CONNECTION_CLOSED = 2;
};

void NetUtils::fill_block_ip(std::string& file_path)
{
  std::ifstream infile(file_path);
  std::string line;
  struct sockaddr_in sa;
  while (std::getline(infile, line)) {
    // Check if line is IP or URI

    if (inet_pton(AF_INET, line.c_str(), &(sa.sin_addr)) == 1) {
      NetUtils::blocked_ip.insert(line);
    } else {
      NetUtils::blocked_host.insert(line);
      std::string ip = NetUtils::resolve_host_name(line);
      NetUtils::blocked_ip.insert(ip);
    }
  }
}
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
    ip = NetUtils::dns_cache[hostname];
  } else {
    struct hostent* lh = gethostbyname(hostname.c_str());
    if (lh) {
      struct in_addr** addr_list;
      addr_list = (struct in_addr**)lh->h_addr_list;
      ip = std::string(inet_ntoa(*addr_list[0]));
    } else {
      herror("Unable to resolve address");
      Utils::print_error_with_message("Unable to resolve address");
      return ip;
    }
    NetUtils::dns_cache[hostname] = ip;
  }
  return ip;
}

void NetUtils::spawn_request_handler(u_short client_socket)
{
  // One thread per request
  int status;
  NetUtils::RqRsHandler rq = NetUtils::RqRsHandler(client_socket);
  //while (true) {
  //std::thread::id this_id = std::this_thread::get_id();
  //std::cout << "DEBUG: Thread ID:" << std::hex << this_id << std::endl;

  debug("Start: Read Request from client");
  status = rq.readRqFromClient();
  debug("End: Read Request from client");

  if (status == NetUtils::RqRsHandler::CONNECTION_CLOSED) {
    debug("Connection Closed");
    //break;
    return;
  } else if (status == RqRsHandler::ERROR) {
    // TODO: handle
    //break;
    return;
  } else if (status == RqRsHandler::HOST_BLOCKED || status == RqRsHandler::IP_BLOCKED || status == RqRsHandler::UNKNOWN_REQUEST || status == RqRsHandler::NO_HOST_RESOLVE) {

    // TODO: Handle;
    debug("Requested Remote is Blocked");
    rq.handleError(status);
    //break;
    return;
  }

  debug("Start: Send Request to Remote");
  status = rq.sendRqToRemote();
  debug("End: Send Request to Remote");

  if (status == RqRsHandler::ERROR) {
    // TODO Handle;
    //break;
    return;
  } else if (status == RqRsHandler::RETURN_CACHE) {
    debug("Sending cached page to client");
    status = rq.sendCacheToClient();
    if (status == RqRsHandler::ERROR) {
      return;
      //break;
      // TODO: Handle better
    }
    debug("Sent cached page to client");
    return;
    //continue;
  }

  debug("Start: Send Response to Client");
  status = rq.sendRsToClient();
  debug("End: Send Response to Client");

  if (status == RqRsHandler::ERROR) {
    debug("Error in sending response to client");
    return;
    //break;
    // TODO Handle better
  } else if (status == RqRsHandler::NO_CONTENT_LENGTH) {
    debug("No Content-Length Supplied in response");
    rq.handleError(status);
    return;
    //break;
  }
  //}
}
void NetUtils::spawn_prefetch_handler(std::string remote_host, std::string remote_ip, u_short port, std::set<std::string> hyperlinks)
{
  for (auto& hyperlink : hyperlinks) {
    // To avoid reading further in case of no content-length supplied
    // TODO: Do not save error pages
    debugs("Extracted Link", hyperlink);
    u_short socket = NetUtils::create_remote_socket(remote_ip, port);
    u_char* buffer;
    std::stringstream request;
    request << "GET /" << hyperlink << " " << httpconstant::http_11;
    request << httpconstant::fields::delim;
    request << "Host: " << remote_host;
    request << httprequest::request_end;
    if (NetUtils::send_to_socket(socket, request.str().c_str(), request.str().length(), "Prefetch: error in sending request") != NetUtils::SUCCESS) {
      close(socket);
      continue;
    }

    std::string header;
    std::vector<u_char> body, buffer_vector;
    NetUtils::recv_headers(socket, header, body);
    header += httpconstant::fields::delim + httpconstant::fields::cache + ": true";
    header += httprequest::request_end;
    ssize_t content_length = NetUtils::RqRsHandler::getContentLengthFromHeader(header);
    if (content_length == NetUtils::RqRsHandler::NO_CONTENT_LENGTH) {

      buffer = (u_char*)malloc(NetUtils::DEFAULT_BUFFLEN * sizeof(u_char));

      while (true) {
        memset(buffer, 0, NetUtils::DEFAULT_BUFFLEN * sizeof(u_char));
        int status = NetUtils::recv_from_socket(socket, (void*)buffer, NetUtils::DEFAULT_BUFFLEN, "Unable to receive content from remote: chunked");
        if (status == NetUtils::ERROR) {
          break;
        }
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
      close(socket);
    } else {

      buffer = (u_char*)malloc(content_length * sizeof(u_char));
      memset(buffer, 0, content_length * sizeof(u_char));
      for (int i = 0; i < body.size(); i++)
        buffer[i] = body[i];

      if (NetUtils::recv_from_socket(socket, (void*)(buffer + body.size()), content_length - body.size(), "Prefetch: unable to recv content") != NetUtils::SUCCESS) {
        free(buffer);
        close(socket);
        continue;
      }
    }
    std::stringstream url_string;
    url_string << remote_host << "http://" << remote_host << "/" << hyperlink;
    debugs("Prefetched URL String", url_string.str());
    std::string url_hash = Utils::get_md5_hash(url_string.str());
    debugs("Prefetched Cache", url_hash);

    Utils::save_to_cache(url_hash, (u_char*)header.c_str(), header.length());
    Utils::save_to_cache(url_hash, buffer, content_length);
  }
}
u_short NetUtils::create_remote_socket(std::string remote_ip, u_short port)
{
  u_short sockfd;
  struct sockaddr_in remote_addr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Utils::print_error_with_message("Unable create remote socket");
    return ERROR;
  }

  memset(&remote_addr, 0, sizeof(struct sockaddr_in));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, remote_ip.c_str(), &remote_addr.sin_addr) <= 0) {
    Utils::print_error_with_message("Invalid remote address");
    return ERROR;
  }

  if (connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr_in)) < 0) {
    Utils::print_error_with_message("Unable to Connect to Remote Server");
    return ERROR;
  }
  return sockfd;
}

int NetUtils::send_to_socket(u_short port, const void* buffer, ssize_t buffer_length, std::string error_msg)
{
  ssize_t s_bytes = 0, t_bytes;
  while (s_bytes != buffer_length) {
    if ((t_bytes = send(port, (u_char*)buffer + s_bytes, buffer_length - s_bytes, 0)) < 0) {
      Utils::print_error_with_message(error_msg);
      return NetUtils::ERROR;
    } else if (t_bytes == 0) {
      debug("Connection Closed");
      break;
    }
    s_bytes += t_bytes;
  }

  return NetUtils::SUCCESS;
}

int NetUtils::recv_from_socket(u_short port, void* buffer, ssize_t buffer_length, std::string error_msg)
{
  ssize_t r_bytes = 0, t_bytes;
  while (r_bytes != buffer_length) {
    if ((t_bytes = recv(port, (u_char*)buffer + r_bytes, buffer_length - r_bytes, 0)) < 0) {
      Utils::print_error_with_message(error_msg);
      return NetUtils::ERROR;
    }
    if (t_bytes == 0) {
      // Connection closed by remote
      return NetUtils::CONNECTION_CLOSED;
    }
    r_bytes += t_bytes;
  }

  return NetUtils::SUCCESS;
}

void NetUtils::recv_headers(u_short socket, std::string& header, std::vector<u_char>& body)
{
  // Header stored doesn't have the request end in it
  ssize_t r_bytes = 0;
  u_char recvbuff[DEFAULT_BUFFLEN];
  memset(recvbuff, 0, (DEFAULT_BUFFLEN) * sizeof(u_char));
  while (true) {
    if ((r_bytes = recv(socket, recvbuff, DEFAULT_BUFFLEN, 0)) < 0) {
      Utils::print_error_with_message("Unable to receive headers from socket");
    }

    if (r_bytes == 0) {
      debug("Connection closed while reading headers");
      break;
    }
    // Searching for HTTP Request End Sequence in the recvbuff
    // Handling case when request_end is sent in two parts
    std::string temp_string(recvbuff, recvbuff + r_bytes);
    header += temp_string;
    size_t idx = header.find(httprequest::request_end);
    if (idx != std::string ::npos) {
      // Header reading done

      // Taking the http body part from header string
      for (size_t i = idx + httprequest::request_end.length(); i < header.length(); i++) {
        body.push_back((u_char)temp_string[i]);
      }
      header = header.substr(0, idx);
      break;
    }

    r_bytes = 0;
  }
}

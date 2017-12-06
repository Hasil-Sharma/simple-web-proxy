#pragma once

namespace httpconstant {
namespace fields {
  const std::string content_length = "Content-Length";
  const std::string content_type = "Content-Type";
  const std::string connection = "Connection";
  const std::string cache = "Cache";
  const std::string host = "Host";
  const std::string delim = "\r\n";
  const std::string connection_close_value = "close";
}
const std::string http_10 = "HTTP/1.0";
const std::string http_11 = "HTTP/1.1";
}
namespace httpresponse {
namespace templates {
  const std::string generic = "<html><body>%s</body></html>\n";
  const std::string error_403 = "<h1>Forbidden</h1><br/>You don't have permission to access %s on this server";
  const std::string error_400 = "<h1>BadRequest</h1><br/>Method not supported: %s";
  const std::string error_404 = "<h1>Not Found</h1><br/>Not Found: %s";
}
namespace header {
  const std::string error_404 = "404 Not Found";
  const std::string error_403 = "403 Forbidden";
  const std::string error_400 = "400 Bad Request";
  const std::string ok_200 = "200 OK";

  const std::string http_10_error_403 = httpconstant::http_10 + " " + error_403;
  const std::string http_11_error_403 = httpconstant::http_11 + " " + error_403;

  const std::string http_10_error_400 = httpconstant::http_10 + " " + error_400;
  const std::string http_11_error_400 = httpconstant::http_11 + " " + error_400;

  const std::string http_10_error_404 = httpconstant::http_10 + " " + error_404;
  const std::string http_11_error_404 = httpconstant::http_11 + " " + error_404;

  const std::string http_10_ok_200 = httpconstant::http_10 + " " + ok_200;
  const std::string http_11_ok_200 = httpconstant::http_11 + " " + ok_200;
}
const std::string error_content_type = "text/html; charset=UTF-8";
const std::string cache_content_type = "text/html; charset=UTF-8";
}

namespace httprequest {
const std::string request_end = "\r\n\r\n";
const std::string type = "GET";
}

#include "utils.h"

void Utils::print_error(std::string s)
{
  std::cerr << s << std::endl;
}

void Utils::print_error_with_message(std::string s)
{
  std::cerr << s << " :" << strerror(errno) << std::endl;
}

std::vector<std::string> Utils::split_string_to_vector(std::string s, const std::string& delimiter)
{
  std::vector<std::string> ans;
  std::string token;
  size_t pos = 0;

  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    ans.push_back(token);
    s.erase(0, pos + delimiter.length());
  }

  // case when last element is the delimiter
  if (s.length())
    ans.push_back(s);
  return ans;
}

int Utils::find_string_index(std::vector<std::string>& strings, std::string& s)
{
  int ans = -1;
  for (size_t i = 0; i < strings.size() && ans < 0; i++) {
    ans = (strings[i].find(s) != std::string::npos) ? i : ans;
  }

  return ans;
}

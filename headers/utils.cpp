#include "utils.h"

namespace Utils {
std::string cache_folder = "cache";
int cache_timeout = 0;
}

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

int Utils::find_string_index(std::vector<std::string>& strings, const std::string& s)
{
  int ans = -1;
  for (size_t i = 0; i < strings.size() && ans < 0; i++) {
    ans = (strings[i].find(s) != std::string::npos) ? i : ans;
  }

  return ans;
}

std::set<std::string> Utils::extract_hyperlinks(std::string& html)
{
  debug("Extracting HTML links");
  static const std::regex hl_regex("<a href=\"(.*?html)\">", std::regex_constants::icase);
  std::set<std::string> html_set = {
    std::sregex_token_iterator(html.begin(), html.end(), hl_regex, 1),
    std::sregex_token_iterator{}
  };

  for (auto str : html_set) {
    debugs("Extracted Links", str);
  }
  return html_set;
}
std::string Utils::generate_dynamic_string(const char** string_templates, int num)
{
  std::string result(string_templates[0]);
  for (int i = 1; i < num; i++) {
    result = std::regex_replace(result, std::regex("%s"), string_templates[i]);
  }
  return result;
}
std::string Utils::get_md5_hash(const std::string& str)
{
  std::string result;
  result.reserve(32);

  MD5_CTX md5;
  MD5_Init(&md5);
  MD5_Update(&md5, (const u_char*)str.c_str(), str.length());
  u_char buffer_md5[MD5_DIGEST_LENGTH];
  MD5_Final(buffer_md5, &md5);

  for (size_t i = 0; i != MD5_DIGEST_LENGTH; i++) {
    result += "0123456789abcdef"[buffer_md5[i] / 16];
    result += "0123456789abcdef"[buffer_md5[i] % 16];
  }

  return result;
}
void Utils::get_path_to_file_cache(const std::string& file_name, std::string& file_path)
{
  file_path = Utils::cache_folder + "/" + file_name;
}
void Utils::save_to_cache(const std::string& file_name, const u_char* buffer, ssize_t buffer_size)
{
  std::string file_path;
  Utils::get_path_to_file_cache(file_name, file_path);
  debugs("File Path", file_path);
  std::ofstream outfile(file_path, std::ofstream::binary | std::ofstream::app);
  outfile.write((const char*)&buffer[0], buffer_size);
  outfile.close();
  debug("Written to Cache");
}
void Utils::set_cache_timeout(int timeout)
{
  Utils::cache_timeout = timeout;
}
bool Utils::check_in_cache(const std::string& file_name)
{
  bool exist_flag, obselete_flag = false;
  std::string file_path;
  Utils::get_path_to_file_cache(file_name, file_path);
  std::ifstream f(file_path.c_str());
  exist_flag = f.good();

  if (!exist_flag)
    return exist_flag;
  struct stat attr;
  stat(file_path.c_str(), &attr);
  time_t last_modification = attr.st_mtime, now;
  time(&now);
  double seconds = difftime(now, last_modification);
  if (seconds > Utils::cache_timeout) {
    obselete_flag = true;
    debugs("Removing the file", file_path);
    remove(file_path.c_str());
  }
  return !obselete_flag;
}

void Utils::read_from_cache(const std::string& file_name, std::vector<u_char>& buffer)
{
  std::string file_path;
  Utils::get_path_to_file_cache(file_name, file_path);
  std::ifstream file(file_path.c_str(), std::ios::binary);

  file.unsetf(std::ios::skipws);
  std::streampos file_size;
  file.seekg(0, std::ios::end);
  file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  buffer.reserve(file_size);

  buffer.insert(buffer.begin(),
      std::istream_iterator<u_char>(file),
      std::istream_iterator<u_char>());
}

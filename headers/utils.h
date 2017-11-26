#include "debug.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <openssl/md5.h>
#include <regex>
#include <set>
#include <vector>

#include <sys/stat.h>
#pragma once
// C++ template to print vector container elements
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
  os << "[";
  for (int i = 0; i < v.size(); ++i) {
    os << v[i];
    if (i != v.size() - 1)
      os << ", ";
  }
  os << "]";
  return os;
}
namespace Utils {
extern std::string cache_folder;
extern int cache_timeout;
void print_error(std::string);
void print_error_with_message(std::string);
std::vector<std::string> split_string_to_vector(std::string, const std::string&);
int find_string_index(std::vector<std::string>&, const std::string&);
std::set<std::string> extract_hyperlinks(std::string&);
std::string generate_dynamic_string(const char**, int);
void save_to_cache(const std::string&, const u_char*, ssize_t);
std::string get_md5_hash(const std::string&);
bool check_in_cache(const std::string&);
void read_from_cache(const std::string&, std::vector<u_char>&);
void get_path_to_file_cache(const std::string&, std::string&);
void set_cache_timeout(int);
}

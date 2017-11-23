#include "debug.h"
#include <cstring>
#include <iostream>
#include <vector>
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
void print_error(std::string);
void print_error_with_message(std::string);
std::vector<std::string> split_string_to_vector(std::string, const std::string&);
int find_string_index(std::vector<std::string>&, const std::string&);
}

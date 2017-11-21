#include <iostream>

#ifndef DEBUG_H
#define DEBUG_H

#define debug(d) std::cerr << "DEBUG: " << d << std::endl;
#define debugs(s, d) std::cerr << "DEBUG: " << s << ": " << d << std::endl;

#endif

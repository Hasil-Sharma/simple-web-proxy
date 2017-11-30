#include <iostream>
#include <thread>
#pragma once
#define debug(d) std::cerr << "DEBUG: Thread ID: " << std::hex << std::this_thread::get_id() << ": " << d << std::endl;
#define debugs(s, d) std::cerr << "DEBUG: Thread ID: " << std::hex << std::this_thread::get_id() << ": " << s << ": " << d << std::endl;

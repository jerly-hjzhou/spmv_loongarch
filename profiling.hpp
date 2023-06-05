#ifndef PROFILING_HPP
#define PROFILING_HPP

#include "env.hpp"
#include <chrono>

DEF_ENV_PARAM(PROFILING)

#define __TIC__(tag) auto tag##_start = std::chrono::steady_clock::now();

#define __TOC__(tag)                                                           \
  auto tag##_end = std::chrono::steady_clock::now();                           \
  if (ENV_PARAM(PROFILING))                                                    \
   std::cout<< "Execution time of [" << #tag << "]: "                        \
              << std::chrono::duration_cast<std::chrono::microseconds>(        \
                     tag##_end - tag##_start)                                  \
                     .count()                                                  \
              << "us" << std::endl;

#endif // PROFILING_HPP
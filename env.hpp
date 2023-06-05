#ifndef ENV_HPP
#define ENV_HPP

#include <cstdlib>

#define DEF_ENV_PARAM(param) \
const char *param = std::getenv(#param);

#define ENV_PARAM(param) ((param) != nullptr && std::atoi(param) == 1)

#endif // ENV_HPP
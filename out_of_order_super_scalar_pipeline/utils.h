//
// Created by itachi on 10/16/17.
//

#ifndef OUT_OF_ORDER_SUPER_SCALAR_PIPELINE_UTILS_H
#define OUT_OF_ORDER_SUPER_SCALAR_PIPELINE_UTILS_H

#include <iostream>
#include <sstream>

namespace utils
{
  static void throw_exception_message(const std::string& message, const std::string& func, uint32_t line)
  {
    std::stringstream str;
    str << std::endl << "Caught Exception In Program : func : " << func << " line : " << line << " **************** \n"
              << message << std::endl << std::endl;
    std::cerr << str.str();
    throw std::exception();
  }

  #define throw_exception(message) throw_exception_message(message, __FILE__, __LINE__)
}
#endif //OUT_OF_ORDER_SUPER_SCALAR_PIPELINE_UTILS_H

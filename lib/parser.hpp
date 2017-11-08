#pragma once

#include "glog/logging.h"
#include "gtest/gtest.h"

#include <cstring>
#include <cstdlib>
#include "boost/utility/string_ref.hpp"

namespace csci5570 {
namespace lib {

template <typename Sample, typename DataStore>
class Parser {
 public:
  /**
   * Parsing logic for one line in file
   *
   * @param line    a line read from the input file
   */
  static Sample parse_libsvm(boost::string_ref line, int n_features) {
    // check the LibSVM format and complete the parsing
    // hints: you may use boost::tokenizer, std::strtok_r, std::stringstream or any method you like
    // so far we tried all the tree and found std::strtok_r is fastest :)
    
    Sample sample;
	
    char *str = strdup(std::string(line).c_str());
    char *saveptr1;
    
    char *token = strtok_r(str, " ", &saveptr1);
    sample.addLabel(atoi(token));
    
    while(true)
    {
        char *key = strtok_r(NULL, ":", &saveptr1);
        if (key==NULL) break;
        char *val = strtok_r(NULL, " ", &saveptr1);
        
       sample.addFeature(std::stoi(key), atof(val));
    }
    
    return sample;
  }

  static Sample parse_mnist(boost::string_ref line, int n_features) {
    // check the MNIST format and complete the parsing
  }

  // You may implement other parsing logic

};  // class Parser

}  // namespace lib
}  // namespace csci5570

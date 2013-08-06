#ifndef _EXCEPTIONS_H
#define _EXCEPTIONS_H

#include <exception>

namespace nodezdb {
class ArgParseException : public std::exception {
private:
  const char* reason;
public:
  ArgParseException(const char* _reason) : reason(_reason) {};
  virtual const char* what() const throw() {
    return reason;
  }
};
}

#endif
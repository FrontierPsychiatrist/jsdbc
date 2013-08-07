/**
The MIT License (MIT)

Copyright (c) 2013 Moritz Schulze 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/
#ifndef _RESULT_SET_H
#define _RESULT_SET_H

#include <node.h>
#include <zdb/zdb.h>

#include <iostream>
using namespace v8;

namespace nodezdb {
class ResultSet : public node::ObjectWrap {
private:
  ResultSet_T resultSet;
  static Persistent<Function> constructor;
  Connection_T connection;
  bool connectionClosed;
public:
  ResultSet(ResultSet_T _resultSet, Connection_T _connection) : resultSet(_resultSet),
    connection(_connection), connectionClosed(false) {};
  ~ResultSet() {
    if(!connectionClosed) {
      Connection_close(connection);
    }
    std::cout << "Result Set dtor called" << std::endl;
  };
  Handle<Object> v8Object() {
    HandleScope scope;
    if(this->handle_.IsEmpty()) {
      Handle<Object> o = constructor->NewInstance();
      this->Wrap(o);
    }
    return scope.Close(handle_);
  };
  static Handle<Value> next(const Arguments& args);
  static Handle<Value> getString(const Arguments& args);
  static Handle<Value> close(const Arguments& args);
  static void init(Handle<Value> target) {
    Local<FunctionTemplate> tpl = FunctionTemplate::New();
    tpl->SetClassName(String::NewSymbol("ResultSet"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(tpl, "next", next);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getString", getString);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    constructor = Persistent<Function>::New(tpl->GetFunction());
  }
};
}
#endif
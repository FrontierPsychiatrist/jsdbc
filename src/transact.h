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
#ifndef _TRANSACT_H
#define _TRANSACT_H

#include <node.h>
#include <zdb/zdb.h>

#include <iostream>

using namespace v8;
namespace nodezdb {

extern ConnectionPool_T pool;

class Transact : public node::ObjectWrap {
private:
  static Handle<Value> query(const Arguments& args);
  static Handle<Value> select(const Arguments& args);
  static Handle<Value> rollback(const Arguments& args);
  static Handle<Value> commit(const Arguments& args);
  static Handle<Value> close(const Arguments& args);
  Connection_T connection;
  static Handle<Function> constructor;
  bool connectionClosed;
public:
  Transact() : connectionClosed(false) {
    Handle<Object> o = constructor->NewInstance();
    this->Wrap(o);
    connection = ConnectionPool_getConnection(pool);
    Connection_beginTransaction(connection);
  };
  ~Transact() {
    if(!connectionClosed) {
      Connection_close(connection);
    }
    std::cout << "The transact dtor has been called" << std::endl;
  }
  Persistent<Object>& v8Object() { return handle_; };
  static void init(Handle<Object> exports) {
    Local<FunctionTemplate> tpl = FunctionTemplate::New();
    tpl->SetClassName(String::NewSymbol("Transact"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(tpl, "query", query);
    NODE_SET_PROTOTYPE_METHOD(tpl, "select", select);
    NODE_SET_PROTOTYPE_METHOD(tpl, "commit", commit);
    NODE_SET_PROTOTYPE_METHOD(tpl, "rollback", rollback);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    constructor = Persistent<Function>::New(tpl->GetFunction());
  }
};
}

#endif
#ifndef _RESULT_SET_H
#define _RESULT_SET_H

#include <node.h>
#include <zdb/zdb.h>

#include <iostream>
using namespace v8;

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

#endif
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
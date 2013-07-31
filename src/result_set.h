#ifndef _RESULT_SET_H
#define _RESULT_SET_H

#include <node.h>
#include <zdb/zdb.h>

using namespace v8;

class ResultSet : public node::ObjectWrap {
private:
  ResultSet_T resultSet;
  static Persistent<Function> constructor;
public:
  ResultSet(ResultSet_T _resultSet) : resultSet(_resultSet) {};
  Persistent<Object>& v8Object() { 
    if(this->handle_.IsEmpty()) {
      Handle<Object> o = constructor->NewInstance();
      this->Wrap(o);
    }
    return handle_;
  };
  static Handle<Value> next(const Arguments& args);
  static Handle<Value> getString(const Arguments& args);
  static void init(Handle<Value> target) {
    Local<FunctionTemplate> tpl = FunctionTemplate::New();
    tpl->SetClassName(String::NewSymbol("ResultSet"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(tpl, "next", next);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getString", getString);
    constructor = Persistent<Function>::New(tpl->GetFunction());
  }
};

#endif
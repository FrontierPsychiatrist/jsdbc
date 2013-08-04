#include "result_set.h"

Persistent<Function> ResultSet::constructor;

Handle<Value> ResultSet::next(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  bool ne(ResultSet_next(resultSet->resultSet) != 0);
  return scope.Close(Boolean::New(ne));
}

Handle<Value> ResultSet::getString(const Arguments& args) {
  HandleScope scope;
  Handle<Number> _i = args[0]->ToNumber();
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  return scope.Close(String::New(ResultSet_getString(resultSet->resultSet, _i->IntegerValue())));
}

Handle<Value> ResultSet::close(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  Connection_close(resultSet->connection);
  resultSet->connectionClosed = true;
  return scope.Close(Undefined());
}
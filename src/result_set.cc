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
#include "result_set.h"

namespace nodezdb {
Persistent<Function> ResultSet::constructor;

Handle<Value> ResultSet::next(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  bool ne(ResultSet_next(resultSet->resultSet) != 0);
  return scope.Close(Boolean::New(ne));
}

Handle<Value> ResultSet::getString(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  Handle<Value> out = Undefined();
  if(args[0]->IsNumber()) {
    Handle<Number> position = args[0]->ToNumber();
    out = String::New(ResultSet_getString(resultSet->resultSet, position->IntegerValue()));
  } else if (args[0]->IsString()) {
    const char* name = *String::Utf8Value(args[0]->ToString());
    out = String::New(ResultSet_getStringByName(resultSet->resultSet, name));
  } else {
    out = ThrowException(Exception::Error(String::New("Only a number or a string is allowed")));
  }
  return scope.Close(out);
}

Handle<Value> ResultSet::getInt(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  Handle<Value> out = Undefined();
  if(args[0]->IsNumber()) {
    Handle<Number> position = args[0]->ToNumber();
    out = Number::New(ResultSet_getInt(resultSet->resultSet, position->IntegerValue()));
  } else if (args[0]->IsString()) {
    const char* name = *String::Utf8Value(args[0]->ToString());
    out = Number::New(ResultSet_getIntByName(resultSet->resultSet, name));
  } else {
    out = ThrowException(Exception::Error(String::New("Only a number or a string is allowed")));
  }
  return scope.Close(out);
}

Handle<Value> ResultSet::getDouble(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  Handle<Value> out = Undefined();
  if(args[0]->IsNumber()) {
    Handle<Number> position = args[0]->ToNumber();
    out = Number::New(ResultSet_getDouble(resultSet->resultSet, position->IntegerValue()));
  } else if (args[0]->IsString()) {
    const char* name = *String::Utf8Value(args[0]->ToString());
    out = Number::New(ResultSet_getDoubleByName(resultSet->resultSet, name));
  } else {
    out = ThrowException(Exception::Error(String::New("Only a number or a string is allowed")));
  }
  return scope.Close(out);
}

Handle<Value> ResultSet::getColumnName(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  Handle<Value> out = Undefined();
  if(args.Length() < 1 || !args[0]->IsNumber()) {
    out = ThrowException(Exception::Error(String::New("Please use a number as the first argument")));
  } else {
    Handle<Number> position = args[0]->ToNumber();
    out = String::New(ResultSet_getColumnName(resultSet->resultSet, position->IntegerValue()));
  }
  return scope.Close(out);
}

Handle<Value> ResultSet::getColumnCount(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  Handle<Value> out = Number::New(ResultSet_getColumnCount(resultSet->resultSet));
  return scope.Close(out);
}

Handle<Value> ResultSet::close(const Arguments& args) {
  HandleScope scope;
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  if(!resultSet->connectionClosed) {
    Connection_close(resultSet->connection);
    resultSet->connectionClosed = true;
  }
  return scope.Close(Undefined());
}
}
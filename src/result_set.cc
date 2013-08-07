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
  Handle<Number> _i = args[0]->ToNumber();
  ResultSet* resultSet = node::ObjectWrap::Unwrap<ResultSet>(args.This());
  return scope.Close(String::New(ResultSet_getString(resultSet->resultSet, _i->IntegerValue())));
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
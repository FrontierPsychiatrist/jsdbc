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
#include <node.h>

#include <zdb/zdb.h>

#include "baton.h"
#include "result.h"
#include "worker_functions.h"
#include "result_set.h"
#include "transact.h"
#include "exceptions.h"

#include <cstring>
#include <iostream>
#include <sstream>

using namespace v8;

namespace nodezdb {
ConnectionPool_T pool;

Baton* createBatonFromArgs(const Arguments& args) throw(ArgParseException) {
  HandleScope scope;

  if(!args[0]->IsString()) {
    ArgParseException exc("First argument must be a string");
    scope.Close(Undefined());
    throw exc;
  }

  String::Utf8Value _query(args[0]->ToString());

  if(args.Length() < 2) {
    //Just a query without a callback, so we can ignore result set parsing
    QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback(*_query);
    baton->request.data = baton;
    scope.Close(Undefined());
    return baton;
  } else if(args[1]->IsArray()) {
    //Prepared Statement
    if(args.Length() < 3) {
      ArgParseException exc("Three arguments are needed");
      scope.Close(Undefined());
      throw exc;
    } else if(!args[2]->IsFunction()) {
      ArgParseException exc("Third argument must be a function");
      scope.Close(Undefined());
      throw exc;
    }

    Handle<Array> values = Handle<Array>::Cast(args[1]);
    Handle<Function> callback = Handle<Function>::Cast(args[2]);
    PreparedStatementBaton* baton = new PreparedStatementBaton(*_query, values->Length(), Persistent<Function>::New(callback));
    for(unsigned int i = 0; i < values->Length(); i++) {
      Local<Value> val = values->Get(i);
      //Todo: make this typed and not only strings
      String::Utf8Value vals(val);
      char* temp = new char[vals.length()];
      memcpy(temp, *vals, vals.length());
      baton->values[i] = temp;
    }
    baton->request.data = baton;
    scope.Close(Undefined());
    return baton;
  } else if(args[1]->IsFunction()) {
    //normal query
    Handle<Function> callback = Handle<Function>::Cast(args[1]);
    QueryBaton* baton = new QueryBaton(Persistent<Function>::New(callback), *_query);
    baton->request.data = baton;
    scope.Close(Undefined());
    return baton;
  } else {
    ArgParseException exc("Unknown function signature. Either use [string], [string ,function] or [string, array, function].");
    scope.Close(Undefined());
    throw exc;
  }
  scope.Close(Undefined());
  return 0;
}

Handle<Value> query(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  try {
    Baton* baton = createBatonFromArgs(args);
    baton->connectionHolder = new StandardConnectionHolder();
    baton->queueWork();
  } catch(ArgParseException e) {
    out = ThrowException(Exception::Error(String::New(e.what())));
  }
  return scope.Close(out);
}

Handle<Value> transact(const Arguments& args) {
  HandleScope scope;
  if(!args[0]->IsFunction()) {
    return scope.Close(ThrowException(Exception::Error(String::New("transact needs a function as its first parameter."))));
  }
  Transact* trans = new Transact();
  Handle<Function> cb = Handle<Function>::Cast(args[0]);
  Handle<Value> argv[] = { trans->v8Object() };
  cb->Call(Context::GetCurrent()->Global(), 1, argv);
  return scope.Close(Undefined());
}

Handle<Value> connect(const Arguments& args) {
  HandleScope scope;
  Handle<Object> params = Handle<Object>::Cast(args[0]);
  const std::string type(*String::Utf8Value(params->Get(String::New("type"))));
  String::Utf8Value _host(params->Get(String::New("host")));
  String::Utf8Value _user(params->Get(String::New("user")));
  String::Utf8Value _password(params->Get(String::New("password")));
  Handle<Number> _port(params->Get(String::New("port"))->ToNumber());
  const std::string database( *String::Utf8Value( params->Get(String::New("database") ) ) );

  std::ostringstream buffer;
  buffer << type << "://" << *_host;
  
  if(type != "sqlite") {
    buffer << ":" << _port->IntegerValue();
  }

  buffer << "/";

  if( database != "undefined" && database != "null" )
  {
    buffer << database;
  }

  if(type != "sqlite") {
    buffer << "?user=" << *_user << "&password=" << *_password;
  }
  
  const std::string connectionString = buffer.str();
  
  URL_T url = URL_new( connectionString.c_str() );
  pool = ConnectionPool_new(url);
  TRY
    ConnectionPool_start(pool);
  CATCH(SQLException)
    return ThrowException(Exception::Error(String::New(Exception_frame.message)));
  END_TRY;
  return scope.Close(Undefined());
}

Handle<Value> stream(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  try {
    BatonWithResult* baton = static_cast<BatonWithResult*>(createBatonFromArgs(args));
    baton->connectionHolder = new StandardConnectionHolder();
    baton->useResultSet = true;
    baton->isSelect = true;
    baton->queueWork();
  } catch(ArgParseException e) {
    out = ThrowException(Exception::Error(String::New(e.what())));
  }
  return scope.Close(out);
}

Handle<Value> select(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  try {
    BatonWithResult* baton = static_cast<BatonWithResult*>(createBatonFromArgs(args));
    baton->connectionHolder = new StandardConnectionHolder();
    baton->isSelect = true;
    baton->queueWork();
  } catch(ArgParseException e) {
    out = ThrowException(Exception::Error(String::New(e.what())));
  }
  return scope.Close(out);
}

void init(Handle<Object> target) {
  target->Set(String::NewSymbol("select"),
              FunctionTemplate::New(select)->GetFunction());
  target->Set(String::NewSymbol("query"),
              FunctionTemplate::New(query)->GetFunction());
  target->Set(String::NewSymbol("connect"),
              FunctionTemplate::New(connect)->GetFunction());
  target->Set(String::NewSymbol("transact"),
              FunctionTemplate::New(transact)->GetFunction());
  target->Set(String::NewSymbol("stream"),
              FunctionTemplate::New(stream)->GetFunction());
  Transact::init(target);
  ResultSet::init(target);
}
NODE_MODULE(jsdbc, init)
}

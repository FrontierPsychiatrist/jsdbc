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
#include "transact.h"
#include "baton.h"
#include "exceptions.h"

namespace nodezdb {
extern Baton* createBatonFromArgs(const Arguments& args);

Handle<Function> Transact::constructor;

Handle<Value> Transact::query(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  try {
    Baton* baton = createBatonFromArgs(args);
    Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());  
    baton->connectionHolder = new TransactionalConnectionHolder(transact->connection);
    baton->queueWork();
  } catch(ArgParseException e) {
    out = ThrowException(Exception::Error(String::New(e.what())));
  }
  return scope.Close(out);
}

Handle<Value> Transact::select(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  try {
    BatonWithResult* baton = static_cast<BatonWithResult*>(createBatonFromArgs(args));
    Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());  
    baton->connectionHolder = new TransactionalConnectionHolder(transact->connection);
    baton->isSelect = true;
    baton->queueWork();
  } catch(ArgParseException e) {
    out = ThrowException(Exception::Error(String::New(e.what())));
  }
  return scope.Close(out);
}

Handle<Value> Transact::rollback(const Arguments& args) {
  HandleScope scope;
  Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());
  Connection_rollback(transact->connection);
  return scope.Close(Undefined()); 
}

Handle<Value> Transact::commit(const Arguments& args) {
  HandleScope scope;
  Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());
  Connection_commit(transact->connection);
  return scope.Close(Undefined());
}

Handle<Value> Transact::close(const Arguments& args) {
  HandleScope scope;
  Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());
  if(!transact->connectionClosed) {
    Connection_close(transact->connection);
    transact->connectionClosed = true;
  }
  return scope.Close(Undefined());
}
}
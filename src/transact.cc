#include "transact.h"
#include "baton.h"

namespace nodezdb {
extern Baton* createBatonFromArgs(const Arguments& args);

Handle<Function> Transact::constructor;

Handle<Value> Transact::query(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  Baton* baton = createBatonFromArgs(args);
  if(baton->creationError != 0) {
    out = ThrowException(Exception::Error(String::New(baton->creationError)));
    delete baton;
  } else {
    Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());  
    baton->connectionHolder = new TransactionalConnectionHolder(transact->connection);
    baton->queueWork();
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
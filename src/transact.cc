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
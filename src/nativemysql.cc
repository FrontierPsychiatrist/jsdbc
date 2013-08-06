#include <node.h>

#include <zdb/zdb.h>

#include "baton.h"
#include "result.h"
#include "worker_functions.h"
#include "result_set.h"

#include <cstring>
#include <iostream>
#include <sstream>

using namespace v8;

ConnectionPool_T pool;

Baton* createBatonFromArgs(const Arguments& args) {
  HandleScope scope;

  if(!args[0]->IsString()) {
    QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback("");
    baton->creationError = "First argument must be a string";
    return baton;
  }

  String::Utf8Value _query(args[0]->ToString());

  if(args.Length() < 2) {
    //Just a query without a callback, so we can ignore result set parsing
    QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback(*_query);
    baton->request.data = baton;
    return baton;
  } else if(args[1]->IsArray()) {
    //Prepared Statement
    if(args.Length() < 3) {
      QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback("");
      baton->creationError = "Three arguments are needed";
      return baton;
    } else if(!args[2]->IsFunction()) {
      QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback("");
      baton->creationError = "Third argument must be a function";
      return baton;
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
    return baton;
  } else if(args[1]->IsFunction()) {
    //normal query
    Handle<Function> callback = Handle<Function>::Cast(args[1]);
    QueryBaton* baton = new QueryBaton(Persistent<Function>::New(callback), *_query);
    baton->request.data = baton;
    return baton;
  } else {
    QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback("");
    baton->creationError = "Unknown function signature. Either use [string], [string ,function] or [string, array, function].";
    return baton;
  }
  scope.Close(Undefined());
  return 0;
}

Handle<Value> query(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  Baton* baton = createBatonFromArgs(args);
  if(baton->creationError != 0) {
    out = scope.Close(ThrowException(Exception::Error(String::New(baton->creationError))));
  } else {
    baton->connectionHolder = new StandardConnectionHolder();
    baton->queueWork();
  }
  return scope.Close(out);
}

class Transact : public node::ObjectWrap {
private:
  static Handle<Value> query(const Arguments& args);
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
    NODE_SET_PROTOTYPE_METHOD(tpl, "commit", commit);
    NODE_SET_PROTOTYPE_METHOD(tpl, "rollback", rollback);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
    constructor = Persistent<Function>::New(tpl->GetFunction());
  }
};

Handle<Function> Transact::constructor;

Handle<Value> Transact::query(const Arguments& args) {
  HandleScope scope;
  Handle<Value> out = Undefined();
  Baton* baton = createBatonFromArgs(args);
  if(baton->creationError != 0) {
    out = ThrowException(Exception::Error(String::New(baton->creationError)));
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
  Connection_close(transact->connection);
  transact->connectionClosed = true;
  return scope.Close(Undefined());
}

Handle<Value> transact(const Arguments& args) {
  HandleScope scope;
  Transact* trans = new Transact();
  Handle<Function> cb = Handle<Function>::Cast(args[0]);
  Handle<Value> argv[] = { trans->v8Object() };
  cb->Call(Context::GetCurrent()->Global(), 1, argv);
  return scope.Close(Undefined());
}

Handle<Value> connect(const Arguments& args) {
  HandleScope scope;
  Handle<Object> params = Handle<Object>::Cast(args[0]);
  String::Utf8Value _host(params->Get(String::New("host")));
  String::Utf8Value _user(params->Get(String::New("user")));
  String::Utf8Value _password(params->Get(String::New("password")));
  Handle<Number> _port(params->Get(String::New("port"))->ToNumber());
  const std::string database( *String::Utf8Value( params->Get(String::New("database") ) ) );

  std::ostringstream buffer;
  buffer << "mysql://" << *_host << ":" << _port->IntegerValue() << "/";

  if( database != "undefined" && database != "null" )
  {
    buffer << database << "/";
  }

  buffer << "?user=" << *_user << "&password=" << *_password;
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
  BatonWithResult* baton = static_cast<BatonWithResult*>(createBatonFromArgs(args));
  if(baton->creationError != 0) {
    out = ThrowException(Exception::Error(String::New(baton->creationError)));
  } else {
    baton->connectionHolder = new StandardConnectionHolder();
    baton->useResultSet = true;
    baton->queueWork();
  }
  return scope.Close(out);
}

void init(Handle<Object> target) {
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
NODE_MODULE(nativemysql, init)

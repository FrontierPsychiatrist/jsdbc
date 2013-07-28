#include <node.h>

#include <zdb/zdb.h>
#include <vector>
#include <string>

#include "baton.h"
#include "result.h"

#include <iostream>

using namespace v8;

ConnectionPool_T pool;

static Result* parseResult(Connection_T* connection, ResultSet_T* result) {
  Result* out;
  int rowsChanged = Connection_rowsChanged(*connection);
  int columnCount = ResultSet_getColumnCount(*result); 
  if(columnCount != 0) {//There are rows
    SelectResult* selectResult = new SelectResult();
    selectResult->fieldNames.resize( columnCount );
    
    for(int i = 1; i <= columnCount; i++) {
      const char* columnName = ResultSet_getColumnName(*result, i);  
      selectResult->fieldNames[i - 1] = columnName;
    } 

    unsigned int rowCounter = 0;
    while( ResultSet_next(*result) ) {
      Row* row = new Row();
      row->fieldValues.resize( columnCount );
      for(int i = 1; i <= columnCount; i++) {
        long size = ResultSet_getColumnSize(*result, i);
        char* temp = new char[ size ];
        memcpy(temp, ResultSet_getString(*result, i), size + 1);
        row->fieldValues[i - 1] = temp;
      }
      selectResult->rows.push_back(row);
      rowCounter++;
    }
    out = selectResult;
  } else {
      UpdateResult* updateResult = new UpdateResult();
      updateResult->affectedRows = rowsChanged;
      out = updateResult;
  }
  return out;
}

static void query(uv_work_t* req) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  int error = 0;
  Connection_T connection;
  if(baton->connection == 0) {
    connection = ConnectionPool_getConnection(pool);
  } else {
    connection = *baton->connection;
  }
  ResultSet_T result;
  TRY
    result = Connection_executeQuery(connection, baton->query.c_str());
  CATCH(SQLException)
    baton->result = new EmptyResult(Connection_getLastError(connection));
    error = 1;
  END_TRY;
  
  if(!error) {
    baton->result = parseResult(&connection, &result);
  }
  if(baton->connection == 0) {
    Connection_close(connection);
  }
}

static void afterQuery(uv_work_t* req, int bla) {
  BatonWithResult* baton = static_cast<BatonWithResult*>(req->data);
  Handle<Value> result = baton->result->getResultObject();
  Handle<Value> error = String::New(baton->result->errorText.c_str());
  Handle<Value> argv[] = { result, error };
  baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
  delete baton;
}

static void queryWithoutResult(uv_work_t* req) {
  QueryBatonWithoutCallback* baton = static_cast<QueryBatonWithoutCallback*>(req->data);
  int error = 0;
  Connection_T connection;
  if(baton->connection == 0) {
    connection = ConnectionPool_getConnection(pool);
  } else {
    connection = *baton->connection;
  }
  ResultSet_T result;
  TRY
    result = Connection_executeQuery(connection, baton->query.c_str());
  CATCH(SQLException)
    error = 1;
    baton->errorText = Connection_getLastError(connection);
  END_TRY;
  if(baton->connection == 0) {
    Connection_close(connection);
  }
}

static void afterQueryWithoutResult(uv_work_t* req, int bla) {
  QueryBatonWithoutCallback* baton = static_cast<QueryBatonWithoutCallback*>(req->data);
  //todo: if error text is not empty throw an error
  delete baton;
}

static void preparedStatement(uv_work_t* req) {
  PreparedStatementBaton* baton = static_cast<PreparedStatementBaton*>(req->data);
  Connection_T connection;
  if(baton->connection == 0) {
    connection = ConnectionPool_getConnection(pool);
  } else {
    connection = *baton->connection;
  }
  PreparedStatement_T pstmt;
  int error = 0;
  TRY
    pstmt = Connection_prepareStatement(connection, baton->query.c_str());
  CATCH(SQLException)
    baton->result = new EmptyResult(Connection_getLastError(connection));
    error = 1;
  END_TRY;

  if(!error) {
    for(int i = 0; i < baton->values.size() && !error; i++) {
      TRY
        PreparedStatement_setString(pstmt, i+1, baton->values[i]);
      CATCH(SQLException)
        error = 1;
        baton->result = new EmptyResult(Connection_getLastError(connection));
      END_TRY;
    }
  }
  ResultSet_T result;
  if(!error) {
    TRY
      result = PreparedStatement_executeQuery(pstmt);
    CATCH(SQLException)
      error = 1;
      baton->result = new EmptyResult(Connection_getLastError(connection));
    END_TRY;
  }

  if(!error) {
    Result* res = parseResult(&connection, &result);
    baton->result = res;
  }
  if(baton->connection == 0) {
    Connection_close(connection);
  }
}

Handle<Value> parseAndFire(const Arguments& args, Connection_T* connection) {
  HandleScope scope;
  
  if(!args[0]->IsString()) {
    return ThrowException(Exception::Error(String::New("First argument must be a string")));
  }

  String::Utf8Value _query(args[0]->ToString());

  if(args.Length() < 2) {
    //Just a query without a callback, so we can ignore result set parsing
    QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback(*_query);
    baton->request.data = baton;
    if(connection != 0) {
      baton->connection = connection;
    }
    uv_queue_work(uv_default_loop(), &baton->request, queryWithoutResult, afterQueryWithoutResult);
  } else if(args[1]->IsArray()) {
    //Prepared Statement
    if(args.Length() < 3) {
      return ThrowException(Exception::Error(String::New("Three arguments are needed")));
    } else if(!args[2]->IsFunction()) {
      return ThrowException(Exception::Error(String::New("Third argument must be a function")));
    }
    
    Handle<Array> values = Handle<Array>::Cast(args[1]);
    Handle<Function> callback = Handle<Function>::Cast(args[2]);
    PreparedStatementBaton* baton = new PreparedStatementBaton(*_query, values->Length(), Persistent<Function>::New(callback));
    for(int i = 0; i < values->Length(); i++) {
      Local<Value> val = values->Get(i);
      //Todo: make this typed and not only strings
      String::Utf8Value vals(val);
      char* temp = new char[vals.length()];
      memcpy(temp, *vals, vals.length());
      baton->values[i] = temp;
    }
    baton->request.data = baton;
    if(connection != 0) {
      baton->connection = connection;
    }
    uv_queue_work(uv_default_loop(), &baton->request, preparedStatement, afterQuery);
  } else if(args[1]->IsFunction()) {
    //normal query
    Handle<Function> callback = Handle<Function>::Cast(args[1]);
    QueryBaton* baton = new QueryBaton(Persistent<Function>::New(callback), *_query);
    baton->request.data = baton;
    if(connection != 0) {
      baton->connection = connection;
    }
    uv_queue_work(uv_default_loop(), &baton->request, query, afterQuery);  
  } else {
    return ThrowException(Exception::Error(String::New("Unknown function signature. Either use [string], [string ,function] or [string, array, function].")));
  }
  
  return scope.Close(Undefined());
}

Handle<Value> query(const Arguments& args) {
  HandleScope scope;
  return scope.Close(parseAndFire(args, 0));
}

class Transact : public node::ObjectWrap {
private:
  static Handle<Value> query(const Arguments& args);
  static Handle<Value> rollback(const Arguments& args);
  static Handle<Value> commit(const Arguments& args);
  static Handle<Value> close(const Arguments& args);
  Connection_T connection;
  static Handle<Function> constructor;
public:
  Transact() {
    Handle<Object> o = constructor->NewInstance();
    this->Wrap(o);
    connection = ConnectionPool_getConnection(pool);
    Connection_beginTransaction(connection);
  };
  ~Transact() {
    std::cout << "The transact dtor has been called" << std::endl;
  }
  Handle<Value> v8Object() { return handle_; };
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
  Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());
  
  //Create query baton WITH attached connection object
  return scope.Close(parseAndFire(args, &transact->connection));
}

Handle<Value> Transact::rollback(const Arguments& args) {
  HandleScope scope;
  Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());
  std::cout << "Rolling back?" << std::endl;
  Connection_rollback(transact->connection);
  Connection_close(transact->connection);
  return scope.Close(Undefined()); 
}

Handle<Value> Transact::commit(const Arguments& args) {
  HandleScope scope;
  Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());
  Connection_commit(transact->connection);
  Connection_close(transact->connection);
  return scope.Close(Undefined());
}

Handle<Value> Transact::close(const Arguments& args) {
  HandleScope scope;
  Transact* transact = node::ObjectWrap::Unwrap<Transact>(args.This());
  Connection_close(transact->connection);
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
  String::Utf8Value _database(params->Get(String::New("database")));

  char buffer[512];
  sprintf(buffer, "mysql://%s:%d/%s?user=%s&password=%s", *_host, _port->IntegerValue(), *_database, *_user, *_password);
  URL_T url = URL_new(buffer);
  pool = ConnectionPool_new(url);
  TRY
    ConnectionPool_start(pool);
  CATCH(SQLException)
    return ThrowException(Exception::Error(String::New(Exception_frame.message)));
  END_TRY;
  return scope.Close(Undefined());
}

void init(Handle<Object> target) {
  target->Set(String::NewSymbol("query"),
              FunctionTemplate::New(query)->GetFunction());
  target->Set(String::NewSymbol("connect"),
              FunctionTemplate::New(connect)->GetFunction());
  target->Set(String::NewSymbol("transact"),
              FunctionTemplate::New(transact)->GetFunction());
  Transact::init(target);
}
NODE_MODULE(nativemysql, init)
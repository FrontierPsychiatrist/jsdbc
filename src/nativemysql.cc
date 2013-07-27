#include <node.h>

#include <zdb/zdb.h>
#include <vector>
#include <string>

#include <iostream>
using namespace v8;

ConnectionPool_T pool;

struct Row {
  std::vector<char*> fieldValues;
  ~Row() {
    for(unsigned int i = 0; i < fieldValues.size(); i++) {
      delete[] fieldValues[i];
    }
  }
};

class Result {
public:
  Result(const char* _errorText) : errorText(_errorText) {};
  Result() {};
  virtual Handle<Value> getResultObject() = 0;
  std::string errorText;
  virtual ~Result() {};
};

class EmptyResult : public Result {
public:
  EmptyResult(const char* errorText) : Result(errorText) {};
  Handle<Value> getResultObject() {
    HandleScope scope;
    return scope.Close(Object::New());
  }
};

class SelectResult : public Result {
public:
  SelectResult() : rows(), fieldNames() {};
  ~SelectResult() {
    for(unsigned int i = 0; i < rows.size(); i++) {
      delete rows[i];
    }
  }
  std::vector<Row*> rows;
  std::vector<std::string> fieldNames;
  Handle<Value> getResultObject() {
    HandleScope scope;
    Handle<Array> array = Array::New();

    for(unsigned int i = 0; i < rows.size(); i++) {
      Handle<Object> obj = Object::New();
      for(unsigned int j = 0; j < rows[i]->fieldValues.size(); j++) {
        obj->Set(String::New(fieldNames[j].c_str()),
          String::New(rows[i]->fieldValues[j]));
      }
      array->Set(Number::New(i), obj);
    }
    return scope.Close(array);
  }
};

class UpdateResult : public Result {
public:
  Handle<Value> getResultObject() {
    HandleScope scope;
    Handle<Object> obj = Object::New();
    obj->Set(String::NewSymbol("affectedRows"), Number::New(affectedRows));
    return scope.Close(obj);
  }
  int affectedRows;
};

struct BatonWithResult {
  BatonWithResult(Persistent<Function> _callback) : callback(_callback) {};
  virtual ~BatonWithResult() {
    delete result;
    callback.Dispose();
  };
  Result* result;
  Persistent<Function> callback;
};

struct QueryBaton : public BatonWithResult {
  QueryBaton(Persistent<Function> _callback, char* _query) :
    BatonWithResult(_callback), request(), query(_query) {};
  uv_work_t request;
  std::string query;
};

struct QueryBatonWithoutCallback {
  QueryBatonWithoutCallback(char* _query) : query(_query) {};
  uv_work_t request;
  std::string query;
  std::string errorText;
};

struct PreparedStatementBaton : public BatonWithResult {
  PreparedStatementBaton(char* _query, int _numValues, Persistent<Function> _callback) :
    BatonWithResult(_callback), query(_query), values(_numValues) {};
  ~PreparedStatementBaton() {
    for(int i = 0; i < values.size(); i++) {
      delete[] values[i];
    }
  }
  uv_work_t request;
  std::string query;
  int _numValues;
  std::vector<char*> values;
};

static Result* parseResult(Connection_T* con, ResultSet_T* result) {
  Result* out;
  int rowsChanged = Connection_rowsChanged(*con);
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
  Connection_T con = ConnectionPool_getConnection(pool);
  ResultSet_T result;
  TRY
    result = Connection_executeQuery(con, baton->query.c_str());
  CATCH(SQLException)
    baton->result = new EmptyResult(Connection_getLastError(con));
    error = 1;
  END_TRY;
  
  if(!error) {
    baton->result = parseResult(&con, &result);
  }
  Connection_close(con);
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
  Connection_T con = ConnectionPool_getConnection(pool);
  ResultSet_T result;
  TRY
    result = Connection_executeQuery(con, baton->query.c_str());
  CATCH(SQLException)
    error = 1;
    baton->errorText = Connection_getLastError(con);
  END_TRY;
  Connection_close(con);
}

static void afterQueryWithoutResult(uv_work_t* req, int bla) {
  QueryBatonWithoutCallback* baton = static_cast<QueryBatonWithoutCallback*>(req->data);
  //todo: if error text is not empty throw an error
  delete baton;
}

static void preparedStatement(uv_work_t* req) {
  PreparedStatementBaton* baton = static_cast<PreparedStatementBaton*>(req->data);
  Connection_T con = ConnectionPool_getConnection(pool);
  PreparedStatement_T pstmt;
  int error = 0;
  TRY
    pstmt = Connection_prepareStatement(con, baton->query.c_str());
  CATCH(SQLException)
    baton->result = new EmptyResult(Connection_getLastError(con));
    error = 1;
  END_TRY;

  if(!error) {
    for(int i = 0; i < baton->values.size() && !error; i++) {
      TRY
        PreparedStatement_setString(pstmt, i+1, baton->values[i]);
      CATCH(SQLException)
        error = 1;
        baton->result = new EmptyResult(Connection_getLastError(con));
      END_TRY;
    }
  }
  ResultSet_T result;
  if(!error) {
    TRY
      result = PreparedStatement_executeQuery(pstmt);
    CATCH(SQLException)
      error = 1;
      baton->result = new EmptyResult(Connection_getLastError(con));
    END_TRY;
  }

  if(!error) {
    Result* res = parseResult(&con, &result);
    baton->result = res;
  }
  Connection_close(con);
}

Handle<Value> query(const Arguments& args) {
  HandleScope scope;
  
  if(!args[0]->IsString()) {
    return ThrowException(Exception::Error(String::New("First argument must be a string")));
  }

  String::Utf8Value _query(args[0]->ToString());

  if(args.Length() < 2) {
    //Just a query without a callback, so we can ignore result set parsing
    QueryBatonWithoutCallback* baton = new QueryBatonWithoutCallback(*_query);
    baton->request.data = baton;
    uv_queue_work(uv_default_loop(), &baton->request, queryWithoutResult, afterQueryWithoutResult);
  }

  if(args[1]->IsArray()) {
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
    uv_queue_work(uv_default_loop(), &baton->request, preparedStatement, afterQuery);
  } else if(args[1]->IsFunction()) {
    //normal query
    Handle<Function> callback = Handle<Function>::Cast(args[1]);
    QueryBaton* baton = new QueryBaton(Persistent<Function>::New(callback), *_query);
    baton->request.data = baton;
    uv_queue_work(uv_default_loop(), &baton->request, query, afterQuery);  
  } else {
    return ThrowException(Exception::Error(String::New("Unknown function signature. Either use [string], [string ,function] or [string, array, function].")));
  }
  
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
}
NODE_MODULE(nativemysql, init)
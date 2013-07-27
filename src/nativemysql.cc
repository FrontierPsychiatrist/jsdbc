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

struct QueryBaton {
  QueryBaton(Persistent<Function> _callback, char* _query) :
    request(), callback(_callback), query(_query) {};
  ~QueryBaton() {
    callback.Dispose();
    delete result;
  }
  uv_work_t request;
  Persistent<Function> callback;
  std::string query;
  Result* result;
};

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
    int rowsChanged = Connection_rowsChanged(con);
    int columnCount = ResultSet_getColumnCount(result); 
    if(columnCount != 0) {//There are rows
      SelectResult* selectResult = new SelectResult();
      selectResult->fieldNames.resize( columnCount );
      
      for(int i = 1; i <= columnCount; i++) {
        const char* columnName = ResultSet_getColumnName(result, i);  
        selectResult->fieldNames[i - 1] = columnName;
      } 

      unsigned int rowCounter = 0;
      while( ResultSet_next(result) ) {
        Row* row = new Row();
        row->fieldValues.resize( columnCount );
        for(int i = 1; i <= columnCount; i++) {
          long size = ResultSet_getColumnSize(result, i);
          char* temp = new char[ size ];
          memcpy(temp, ResultSet_getString(result, i), size + 1);
          row->fieldValues[i - 1] = temp;
        }
        selectResult->rows.push_back(row);
        rowCounter++;
      }
      baton->result = selectResult;
    } else {
        UpdateResult* updateResult = new UpdateResult();
        updateResult->affectedRows = rowsChanged;
        baton->result = updateResult;
    }
  }
  Connection_close(con);
}

static void afterQuery(uv_work_t* req, int bla) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  Handle<Value> result = baton->result->getResultObject();
  Handle<Value> error = String::New(baton->result->errorText.c_str());
  Handle<Value> argv[] = { result, error };
  baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
  delete baton;
}

Handle<Value> query(const Arguments& args) {
  HandleScope scope;
  String::Utf8Value _query(args[0]->ToString());
  Handle<Function> callback = Handle<Function>::Cast(args[1]);
  QueryBaton* baton = new QueryBaton(Persistent<Function>::New(callback), *_query);
  baton->request.data = baton;
  uv_queue_work(uv_default_loop(), &baton->request, query, afterQuery);
  return scope.Close(Undefined());
}

Handle<Value> connect(const Arguments& args) {
  HandleScope scope;
  Handle<Object> params = Handle<Object>::Cast(args[0]);
  String::Utf8Value _host(params->Get(String::New("host")));
  String::Utf8Value _user(params->Get(String::New("user")));
  String::Utf8Value _password(params->Get(String::New("password")));
  Number _port(**params->Get(String::New("port"))->ToNumber());
  String::Utf8Value _database(params->Get(String::New("database")));

  char buffer[100];
  sprintf(buffer, "mysql://%s:%d/%s?user=%s&password=%s", *_host, (int)_port.IntegerValue(), *_database, *_user, *_password);
  URL_T url = URL_new(buffer);
  pool = ConnectionPool_new(url);
  ConnectionPool_start(pool);
  return scope.Close(Undefined());
}

void init(Handle<Object> target) {
  target->Set(String::NewSymbol("query"),
              FunctionTemplate::New(query)->GetFunction());
  target->Set(String::NewSymbol("connect"),
              FunctionTemplate::New(connect)->GetFunction());
}
NODE_MODULE(nativemysql, init)
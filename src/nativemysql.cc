#include <node.h>

#include <mysql.h>
#include <vector>
#include <string>
#include <pthread.h>

#include <iostream>
using namespace v8;

MYSQL mysql;
pthread_mutex_t mutex;

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

/**
struct INSERT,UPDATE,DELETEResult {
  
};
**/

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
  pthread_mutex_lock(&mutex);
  int error = mysql_query(&mysql, baton->query.c_str());

  if(error) {
    baton->result = new EmptyResult(mysql_error(&mysql));
  } else {
    MYSQL_RES* result;
    result = mysql_store_result(&mysql);
    if(result) {//There are rows
      SelectResult* selectResult = new SelectResult();
      selectResult->rows.resize(mysql_num_rows(result));
      selectResult->fieldNames.resize(mysql_num_fields(result));

      MYSQL_FIELD* mysqlField;
      unsigned int fieldCounter = 0;
      while((mysqlField = mysql_fetch_field(result))) {
        selectResult->fieldNames[fieldCounter] = mysqlField->name;
        fieldCounter++;
      }

      MYSQL_ROW mysqlRow;
      unsigned int rowCounter = 0;
      unsigned long* lengths;
      while((mysqlRow = mysql_fetch_row(result))) {
        lengths = mysql_fetch_lengths(result);
        Row* row = new Row();
        row->fieldValues.resize(mysql_num_fields(result));
        for(unsigned int i = 0; i < mysql_num_fields(result); i++) {
          char* temp = new char[lengths[i]];
          memcpy(temp, mysqlRow[i], lengths[i]);
          row->fieldValues[i] = temp;
        }
        selectResult->rows[rowCounter] = row;
        rowCounter++;
      }
      mysql_free_result(result);
      baton->result = selectResult;
    } else {//There isn't a result set, so it wasn't a select
      if(mysql_field_count(&mysql) == 0) {
        UpdateResult* updateResult = new UpdateResult();
        updateResult->affectedRows = mysql_affected_rows(&mysql);
        baton->result = updateResult;
      } else {
        //mysql_store_result should have returned data
        baton->result = new EmptyResult("There should have been data");
      }
    }
  }
  pthread_mutex_unlock(&mutex);
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
  
  mysql_init(&mysql);
  if(!mysql_real_connect(&mysql, *_host, *_user, *_password, *_database, _port.IntegerValue(), 0, 0)) {
    return ThrowException(Exception::Error(String::New(mysql_error(&mysql))));
  }
  return scope.Close(Undefined());
}

void init(Handle<Object> target) {
  target->Set(String::NewSymbol("query"),
              FunctionTemplate::New(query)->GetFunction());
  target->Set(String::NewSymbol("connect"),
              FunctionTemplate::New(connect)->GetFunction());
  pthread_mutex_init(&mutex, NULL);
}
NODE_MODULE(nativemysql, init)
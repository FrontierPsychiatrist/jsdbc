#include <node.h>

#include <mysql.h>
#include <vector>
#include <string>

#include <iostream>
using namespace v8;

MYSQL mysql;

struct Row {
  std::vector<std::string> fieldValues;
};

struct QueryBaton {
  QueryBaton(Persistent<Function> _callback, char* _query) :
    request(), callback(_callback), query(_query), rows(), fieldNames(), errorText() {};
  ~QueryBaton() {
    for(unsigned int i = 0; i < rows.size(); i++) {
      delete rows[i];
    }
    callback.Dispose();
  }
  uv_work_t request;
  Persistent<Function> callback;
  std::string query;
  std::vector<Row*> rows;
  std::vector<std::string> fieldNames;
  std::string errorText;
};

static void query(uv_work_t* req) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  int error = mysql_query(&mysql, baton->query.c_str());

  if(error) {
    baton->errorText = mysql_error(&mysql);
  } else {
    MYSQL_RES* result;
    result = mysql_store_result(&mysql);
    if(result) {
      baton->rows.resize(mysql_num_rows(result));
      baton->fieldNames.resize(mysql_num_fields(result));

      MYSQL_FIELD* mysqlField;
      
      while((mysqlField = mysql_fetch_field(result))) {
        baton->fieldNames.push_back(mysqlField->name);
      }

      MYSQL_ROW mysqlRow;
      unsigned int rowCounter = 0;
      unsigned long* lengths;
      while((mysqlRow = mysql_fetch_row(result))) {
        lengths = mysql_fetch_lengths(result);
        Row* row = new Row();
        row->fieldValues.resize(mysql_num_fields(result));
        for(unsigned int i = 0; i < mysql_num_fields(result); i++) {
          row->fieldValues.push_back(mysqlRow[i]);
        }
        baton->rows[rowCounter] = row;
        rowCounter++;
      }
      mysql_free_result(result);
    }
  }
}

static void afterQuery(uv_work_t* req, int bla) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  Handle<Array> array = Array::New();

  for(unsigned int i = 0; i < baton->rows.size(); i++) {
    Handle<Object> obj = Object::New();
    for(unsigned int j = 0; j < baton->fieldNames.size(); j++) {
      obj->Set(String::New(baton->fieldNames[j].c_str()),
        String::New(baton->rows[i]->fieldValues[j].c_str()));
    }
    array->Set(Number::New(i), obj);
  }

  Handle<Value> error = String::New(baton->errorText.c_str());
  Handle<Value> argv[] = { array, error };
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
}
NODE_MODULE(nativemysql, init)
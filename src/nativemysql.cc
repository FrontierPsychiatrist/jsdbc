#include <node.h>

#include <mysql.h>

#include <iostream>
using namespace v8;

MYSQL mysql;

struct QueryBaton {
  uv_work_t request;
  Persistent<Function> callback;
  char** ids;
};

static void query(uv_work_t* req) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  int error = mysql_query(&mysql, "SELECT m_id FROM pxm_message");
  
  if(error) {
    baton->ids = 0;
  } else {
    MYSQL_RES* result;
    result = mysql_store_result(&mysql);
    baton->ids = (char**)malloc( 20 * sizeof(char*));
    if(result) {
      MYSQL_ROW row;
      unsigned int i = 0;
      while((row = mysql_fetch_row(result))) {
        baton->ids[i] = row[0];
        i++;
      }
      mysql_free_result(result);
    }
  }
}

static void afterQuery(uv_work_t* req, int bla) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  Handle<Array> array = Array::New();
  for(int i = 0; i < 20; i++) {
    array->Set(Number::New(i), String::New(baton->ids[i]));
  }
  Handle<Value> argv[] = { array };
  baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
  baton->callback.Dispose();
  free(baton->ids);
  delete baton;
}

Handle<Value> query(const Arguments& args) {
  HandleScope scope;
  Handle<Function> callback = Handle<Function>::Cast(args[0]);
  QueryBaton* baton = new QueryBaton();
  baton->request.data = baton;
  baton->callback = Persistent<Function>::New(callback);
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
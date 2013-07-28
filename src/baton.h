#ifndef _BATONS_H
#define _BATONS_H

#include <v8.h>
#include <uv.h>
#include <zdb/zdb.h>
#include <string>
#include <vector>

#include "result.h"

using namespace v8;

extern ConnectionPool_T pool;

struct Baton {
  Baton(char* _query) : query(_query) {};
  std::string query;
  uv_work_t request;
  Connection_T* connection;
};

struct BatonWithResult : public Baton {
  BatonWithResult(Persistent<Function> _callback, char* _query) : Baton(_query), callback(_callback) {};
  virtual ~BatonWithResult() {
    delete result;
    callback.Dispose();
  };
  Result* result;
  Persistent<Function> callback;
};

struct QueryBaton : public BatonWithResult {
  QueryBaton(Persistent<Function> _callback, char* _query) :
    BatonWithResult(_callback, _query) {};
};

struct QueryBatonWithoutCallback : Baton {
  QueryBatonWithoutCallback(char* _query) : Baton(_query) {};
  std::string errorText;
};

struct PreparedStatementBaton : public BatonWithResult {
  PreparedStatementBaton(char* _query, int _numValues, Persistent<Function> _callback) :
    BatonWithResult(_callback, _query), values(_numValues) {};
  ~PreparedStatementBaton() {
    for(int i = 0; i < values.size(); i++) {
      delete[] values[i];
    }
  }
  int _numValues;
  std::vector<char*> values;
};

#endif
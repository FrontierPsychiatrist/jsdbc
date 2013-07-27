#ifndef _BATONS_H
#define _BATONS_H

#include <v8.h>
#include <uv.h>
#include <string>
#include <vector>

#include "result.h"

using namespace v8;

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

#endif
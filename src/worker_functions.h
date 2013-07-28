#ifndef _WORKER_FUNCTIONS
#define _WORKER_FUNCTIONS

#include <uv.h>

void queryWork(uv_work_t* req);
void afterQuery(uv_work_t* req, int bla);
void queryWithoutResult(uv_work_t* req);
void afterQueryWithoutResult(uv_work_t* req, int bla);
void preparedStatement(uv_work_t* req);

#endif
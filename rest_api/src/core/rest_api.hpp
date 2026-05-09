#ifndef __REST_API_
#define __REST_API_

#include <mysql++/mysql++.h>
#include <sw/redis++/redis++.h>
#include <iostream>
#include <string>
#include <mysql_conn_pool.hpp>
#include <config_loader.hpp>
// call repos here

#define MYSQLPP_SSQLS_NO_STATICS
#include <court_doc_repository.hpp>
#undef MYSQLPP_SSQLS_NO_STATICS


const mysql_connection_t* allocate_mysql_credentials(const char* db, const char* server=0, const char* user=0, const char* password=0, unsigned int port=0);
int simple_api(const mysql_connection_t* conn_id, mysql_simple_func_ptr_t<> func_ptr_arr[], int port);
int thread_safe_api(const mysql_connection_t* conn_id, mysql_thread_safe_func_ptr_t<> func_ptr_arr[], int port);
int thread_safe_cors_api(const mysql_connection_t* conn_id, mysql_thread_safe_cors_func_ptr_t<> func_ptr_arr[], int port, const char* allowed_origins);
int free_mysql_credentials(mysql_connection_t* connection_param);


#endif

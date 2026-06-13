#ifndef __REST_API_
#define __REST_API_

#include <mysql++/mysql++.h>
#include <rest_api/mysql_conn_pool.hpp>
#include <sw/redis++/redis++.h>
#include <iostream>
#include <string>
#include <rest_api/config_loader.hpp>

const mysql_connection_t* allocate_mysql_credentials(const char* db, const char* server=0, const char* user=0, const char* password=0, unsigned int port=0);
int simple_api(const mysql_connection_t* conn_id, mysql_simple_func_ptr_t<> func_ptr_arr[], int port);
int thread_safe_api(const mysql_connection_t* conn_id, mysql_thread_safe_func_ptr_t<> func_ptr_arr[], int port);
int thread_safe_cors_api(const mysql_connection_t* conn_id, mysql_thread_safe_cors_func_ptr_t<> func_ptr_arr[], int port, const char* allowed_origins, const char* bind_addr = nullptr);
int free_mysql_credentials(mysql_connection_t* connection_param);


#endif

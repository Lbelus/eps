#ifndef __REST_API_
#define __REST_API_

#include <mysql++/mysql++.h>
#include <sw/redis++/redis++.h>
#include <iostream>
#include <string>
#include <mysql_conn_pool.hpp>

// call repos here

#include <mysql_string_repository.hpp>
#define MYSQLPP_SSQLS_NO_STATICS
#include <example_repository.hpp>
#undef MYSQLPP_SSQLS_NO_STATICS
// helper

// crow::json::wvalue to_crow_json(const std::vector<mysqlpp::Row>& rows, const std::vector<std::string>& names);

const mysql_connection_t* allocate_mysql_credentials(const char* db, const char* server=0, const char* user=0, const char* password=0, unsigned int port=0);
int simple_api(const mysql_connection_t* conn_id, mysql_simple_func_ptr_t<> func_ptr_arr[], int port);
int thread_safe_api(const mysql_connection_t* conn_id, mysql_thread_safe_func_ptr_t<> func_ptr_arr[], int port);
int free_mysql_credentials(mysql_connection_t* connection_param);

// // mysql routes:
// //
// void simple_crow_get_all_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// void simple_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// void simple_crow_insert_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// void simple_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// void simple_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// void simple_crow_get_joined_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// void simple_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

// void thread_safe_crow_get_all_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);
// void thread_safe_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);
// void thread_safe_crow_insert_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);
// void thread_safe_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);
// void thread_safe_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);
// void thread_safe_crow_get_joined_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);
// void thread_safe_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);

// // redis routes:

// //post
// void simple_crow_set(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_lpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_rpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_hmget(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);

// // get
// void simple_crow_key(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_get(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_lpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_rpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_llen(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_hmset(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_ping(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_echo(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_flushall(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void simple_crow_info(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);



// // void thread_safe_crow_get_all_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// // void thread_safe_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// // void thread_safe_crow_insert_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// // void thread_safe_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// // void thread_safe_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// // void thread_safe_crow_get_joined_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
// // void thread_safe_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

// // redis routes:

// //post
// void thread_safe_crow_set(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_lpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_rpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_hmget(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);

// // get
// void thread_safe_crow_key(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_get(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_lpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_rpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_llen(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_hmset(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_ping(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_echo(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_flushall(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);
// void thread_safe_crow_info(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis);

#endif

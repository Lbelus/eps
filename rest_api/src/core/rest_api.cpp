#include <stdio.h>
#include <string.h>
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <mysql_conn_pool.hpp>

const char* allocate_string(const char* str)
{
    char* new_str = NULL; 
    if (str != NULL)
    {
        size_t len = strlen(str) + 1;
        new_str = (char*)malloc(sizeof(char)* len);
        strcpy(new_str, str);
    }
    return (const char*)new_str;
}

const mysql_connection_t* allocate_mysql_credentials(
    const char* db, 
    const char* server, 
    const char* user, 
    const char* password, 
    unsigned int port)
{
    mysql_connection_t* new_conn_param = (mysql_connection_t*)malloc(sizeof(mysql_connection_t));
    new_conn_param->db = allocate_string(db);
    new_conn_param->server = allocate_string(server);
    new_conn_param->user = allocate_string(user);
    new_conn_param->password = allocate_string(password);
    new_conn_param->port = port;
    return  (const mysql_connection_t*) new_conn_param;
}

int free_mysql_credentials(mysql_connection_t* connection_param)
{
    free((void*)connection_param->db); 
    free((void*)connection_param->server); 
    free((void*)connection_param->user); 
    free((void*)connection_param->password);
    free(connection_param);
    return EXIT_SUCCESS;
}

int simple_api(const mysql_connection_t* conn_id, mysql_simple_func_ptr_t<> func_ptr_arr[], int port)
{
    crow::SimpleApp app;
    mysqlpp::Connection conn(conn_id->db, conn_id->server, conn_id->user, conn_id->password);
    size_t index = 0;
    while (func_ptr_arr[index] != NULL)
    {
        func_ptr_arr[index](app, conn);
        index += 1;
    }
    app.port(port).run();
    return EXIT_SUCCESS;
}

int thread_safe_api(const mysql_connection_t* conn_id, mysql_thread_safe_func_ptr_t<> func_ptr_arr[], int port)
{
    crow::SimpleApp app;
    SimpleConnectionPool pool(conn_id);
    if (!mysqlpp::Connection::thread_aware())
    {
        std::cerr << "MySQL++/libmysqlclient not built thread-aware on this system\n";
        return 1;
    }
    size_t index = 0;
    while (func_ptr_arr[index] != NULL)
    {
        func_ptr_arr[index](app, pool);
        index += 1;
    }
    app.port(port).multithreaded().run();
    return EXIT_SUCCESS;
}

int thread_safe_cors_api(const mysql_connection_t* conn_id, mysql_thread_safe_cors_func_ptr_t<> func_ptr_arr[], int port, const char* allowed_origins, const char* bind_addr)
{
    crow::App<crow::CORSHandler> app;
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization", "Accept", "Origin", "X-Requested-With")
        .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .origin(allowed_origins);
    SimpleConnectionPool pool(conn_id);
    if (!mysqlpp::Connection::thread_aware())
    {
        std::cerr << "MySQL++/libmysqlclient not built thread-aware on this system\n";
        return 1;
    }
    size_t index = 0;
    while (func_ptr_arr[index] != NULL)
    {
        func_ptr_arr[index](app, pool);
        index += 1;
    }
    if (bind_addr != nullptr && bind_addr[0] != '\0')
    {
        app.bindaddr(bind_addr).port(port).multithreaded().run();
    }
    else
    {
        app.port(port).multithreaded().run();
    }
    return EXIT_SUCCESS;
}

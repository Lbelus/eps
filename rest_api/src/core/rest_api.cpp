#include <stdio.h>
#include <string.h>
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <rest_api/rest_api.hpp>
#include <rest_api/route_registry.hpp>

#include <cstdlib>
#include <stdexcept>

namespace
{
using RestApiApp = crow::App<crow::CORSHandler>;

void configure_cors(RestApiApp& app, const server_config_t& config)
{
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization", "Accept", "Origin", "X-Requested-With")
        .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .origin(config.allowed_origins);
}

void bind_routes_for_db(RestApiApp& app, const db_config_t& config, IConnectionPool& pool)
{
    switch (config.type)
    {
        case DbType::MySQL:
        {
            auto* mysql_pool = dynamic_cast<MySqlConnectionPool*>(&pool);
            if (mysql_pool == nullptr)
            {
                throw std::runtime_error("Connection pool type mismatch for MySQL config: " + config.name);
            }

            for (const auto& route_name : config.route_vec)
            {
                RestApiMySqlRouteRegistry::instance().bind(route_name, app, mysql_pool->pool());
            }
            return;
        }

        case DbType::Redis:
            throw std::runtime_error("Redis route binding is not implemented yet: " + config.name);

        case DbType::PostgreSQL:
            throw std::runtime_error("PostgreSQL route binding is not implemented yet: " + config.name);

        case DbType::unknown:
            break;
    }

    throw std::runtime_error("Unknown database type for route binding: " + config.name);
}
}

rest_api::rest_api(const std::string& config_file)
    : app_config(load_config(config_file.c_str()))
{}

int rest_api::start()
{
    pool_reg.clear();

    RestApiApp app;
    configure_cors(app, app_config.server_config);

    for (const auto& db_config : app_config.db_config_vec)
    {
        const ConnectionPoolRegistry::Id id = pool_reg.create_and_add(db_config);
        IConnectionPool& pool = pool_reg.get(id);

        if (!pool.connect())
        {
            return EXIT_FAILURE;
        }

        bind_routes_for_db(app, db_config, pool);
    }

    if (!app_config.server_config.bind_addr.empty())
    {
        app.bindaddr(app_config.server_config.bind_addr)
            .port(app_config.server_config.port)
            .multithreaded()
            .run();
    }
    else
    {
        app.port(app_config.server_config.port)
            .multithreaded()
            .run();
    }

    return EXIT_SUCCESS;
}

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

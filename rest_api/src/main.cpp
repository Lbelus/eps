#include <rest_api.hpp>

int main()
{
    app_config_t config = load_config("./config.yaml");
    const mysql_connection_t* creds = allocate_mysql_credentials(config.mysql.database.c_str(), config.mysql.host.c_str(), config.mysql.user.c_str(), config.mysql.password.c_str());
    mysql_thread_safe_cors_func_ptr_t<> func_ptr_arr[] = 
    {
            &mysqlCourtDocuments_routes,
            NULL
    };

    thread_safe_cors_api(creds, func_ptr_arr, config.server.port, config.server.allowed_origins.c_str(), config.server.bind_addr.c_str());
    free_mysql_credentials((mysql_connection_t*)creds);
    return EXIT_SUCCESS;
}

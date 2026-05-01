#include <rest_api.hpp>

int main()
{
    app_config_t config = load_config("./config.yaml");
    const mysql_connection_t* creds = allocate_mysql_credentials(config.mysql.host.c_str(), config.server.host.c_str(), config.mysql.user.c_str(), config.mysql.password.c_str());
    // mysql_simple_func_ptr_t func_ptr_arr[] =
    // {
    //         &simple_crow_get_all_entity,
    //         &simple_crow_get_entity_by_id,
    //         &simple_crow_insert_entity,
    //         &simple_crow_update_entity_by_id,
    //         &simple_crow_delete_entity_by_id,
    //         &simple_crow_get_joined_entities,
    //         &simple_crow_get_ordered_entities,
    //         NULL
    // };
    // simple_api_exec(creds, func_ptr_arr, 3004);

    // mysql_thread_safe_func_ptr_t<> func_ptr_arr[] =
    // {
    //         // &thread_safe_crow_get_all_entity,
    //         // &thread_safe_crow_get_entity_by_id,
    //         // &thread_safe_crow_insert_entity,
    //         // &thread_safe_crow_update_entity_by_id,
    //         // &thread_safe_crow_delete_entity_by_id,
    //         // &thread_safe_crow_get_joined_entities,
    //         // &thread_safe_crow_get_ordered_entities,
    //         // &mysqlExampleUsers_routes,
    //         &mysqlCourtDocuments_routes,
    //         NULL
    // };
    mysql_thread_safe_cors_func_ptr_t<> func_ptr_arr[] = 
    {
            // &thread_safe_crow_get_all_entity,
            // &thread_safe_crow_get_entity_by_id,
            // &thread_safe_crow_insert_entity,
            // &thread_safe_crow_update_entity_by_id,
            // &thread_safe_crow_delete_entity_by_id,
            // &thread_safe_crow_get_joined_entities,
            // &thread_safe_crow_get_ordered_entities,
            // &mysqlExampleUsers_routes,
            &mysqlCourtDocuments_routes,
            NULL
    };

    thread_safe_cors_api(creds, func_ptr_arr, config.server.port, config.server.allowed_origins.c_str());
    free_mysql_credentials((mysql_connection_t*)creds);
    return EXIT_SUCCESS;
}

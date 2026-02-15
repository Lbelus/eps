#include <rest_api.hpp>

int main()
{
    const mysql_connection_t* creds = allocate_mysql_credentials("test_rest_DB", "mysqlserver", "dev_admin", "dev_admin");
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

    mysql_thread_safe_func_ptr_t<> func_ptr_arr[] =
    {
            &thread_safe_crow_get_all_entity,
            &thread_safe_crow_get_entity_by_id,
            &thread_safe_crow_insert_entity,
            &thread_safe_crow_update_entity_by_id,
            &thread_safe_crow_delete_entity_by_id,
            &thread_safe_crow_get_joined_entities,
            &thread_safe_crow_get_ordered_entities,
            &mysqlExampleUsers_routes,
            NULL
    };

    thread_safe_api(creds, func_ptr_arr, 3004);
    free_mysql_credentials((mysql_connection_t*)creds);
    return EXIT_SUCCESS;
}

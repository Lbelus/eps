#include <mysql_string_repository.hpp>

// Query acts as a standard C++ output stream, so you can write data to it like you would to std::cout or std::ostringstream.
// The library includes stream manipulators that are type-aware.
// source: https://tangentsoft.com/mysqlpp/doc/html/userman/overview.html

    //
bool MySqlStringRepository::insert(const std::string& table_name, const std::string& keys, const std::string& values)
{
    mysqlpp::Query query = conn().query(); 
    query << "INSERT INTO `" << table_name << "` "
        << keys
        << " VALUES "
        << values; 
    simple_result = query.execute();
    if (simple_result)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        error_msg = query.error();
        return EXIT_FAILURE;
    } 
}

void MySqlStringRepository::store_names(const mysqlpp::StoreQueryResult& query_result)
{
    names.clear();
    names.reserve(query_result.num_fields());
    size_t index = 0; 
    while (index < query_result.num_fields())
    {
        names.emplace_back(query_result.field_name(index));
        index += 1;
    } 
}

//R
bool MySqlStringRepository::select_all(const std::string& key)
{
    mysqlpp::Query query = conn().query();
    query << "SELECT * FROM `" << key << "`";
    mysqlpp::StoreQueryResult query_result = query.store();
    if (query_result)
    {
        store_names(query_result);
        rows.assign(query_result.begin(), query_result.end());
        return EXIT_SUCCESS;
    }
    else
    {
        error_msg = query.error();
        return EXIT_FAILURE;
    }
}



bool MySqlStringRepository::select_by_id(const std::string& key, int id)
{
    mysqlpp::Query query = conn().query();
    query << "SELECT * FROM `" << key << "` WHERE id=" << id;
    mysqlpp::StoreQueryResult query_result = query.store();
    if (query_result)
    {
        store_names(query_result);
        rows.assign(query_result.begin(), query_result.end());
        return EXIT_SUCCESS;
    }
    else
    {
        error_msg = query.error();
        return EXIT_FAILURE;
    }
}

bool MySqlStringRepository::join(const std::string& table_name_A, const std::string& table_name_B)
{
    mysqlpp::Query query = conn().query();
    query << "SELECT * FROM `" << table_name_A << "` " 
        << "JOIN `" << table_name_B << "` "
        << "ON `" << table_name_A << "`.`id` = `" << table_name_B << "`.`" << table_name_A << "_id`";
    mysqlpp::StoreQueryResult query_result = query.store();
    if (query_result)
    {
        store_names(query_result);
        rows.assign(query_result.begin(), query_result.end());
        return EXIT_SUCCESS;
    }
    else
    {
        error_msg = query.error();
        return EXIT_FAILURE;
    }
}

bool MySqlStringRepository::order(const std::string& table_name, const std::string& column, const std::string& order)
{
    mysqlpp::Query query = conn().query();
    query << "SELECT * FROM `" << table_name + "` "
        << "ORDER BY `" << column << "` " << order;
    mysqlpp::StoreQueryResult query_result = query.store();
    if (query_result)
    {
        store_names(query_result);
        rows.assign(query_result.begin(), query_result.end());
        return EXIT_SUCCESS;
    }
    else
    {
        error_msg = query.error();
        return EXIT_FAILURE;
    }
}

//U
bool MySqlStringRepository::update(const std::string& table_name, const std::string& fields_string, int id)
{
    mysqlpp::Query query = conn().query(); 
    query << "UPDATE `" << table_name << "` SET " << fields_string << " WHERE `id` = " << id;
    simple_result = query.execute();
    if (simple_result)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        error_msg = query.error();
        return EXIT_FAILURE;
    } 
}

//D
bool MySqlStringRepository::delete__(const std::string& table_name, int id)
{
    mysqlpp::Query query = conn().query();
    query << "DELETE FROM `" << table_name << "` WHERE `id` =" << id;
    simple_result = query.execute();
    if (simple_result)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        error_msg = query.error();
        return EXIT_FAILURE;
    } 
}
// getter 
const char* MySqlStringRepository::error()
{
    return error_msg;
}

const std::vector<mysqlpp::Row>& MySqlStringRepository::get_rows()
{
    return rows;
}
const std::vector<std::string>& MySqlStringRepository::get_names()
{
    return names;
}

// // thread safe routes
// template <typename... Middlewares>
// void thread_safe_crow_get_all_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
// {
//     CROW_ROUTE(app, "/read/<string>").methods(crow::HTTPMethod::GET)
//     ([&pool_ptr](const std::string& key)
//     {
//         mysqlpp::ScopedConnection sc(pool_ptr, true);
//         MySqlStringRepositoryImpl concrete(sc);
//         IStringRepository& repo = concrete; 
//         bool result = repo.select_all(key);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }


// template <typename... Middlewares>
// void thread_safe_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
// {
//     CROW_ROUTE(app, "/read/<string>/<int>").methods(crow::HTTPMethod::GET)
//     ([&pool_ptr](const std::string& key, int id)
//     {
//          
//         mysqlpp::ScopedConnection sc(pool_ptr, true);
//         MySqlStringRepositoryImpl concrete(sc);
//         IStringRepository& repo = concrete; 
//         bool result = repo.select_by_id(key, id);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }

// // JOIN
// template <typename... Middlewares>
// void thread_safe_crow_get_joined_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
// {
//     CROW_ROUTE(app, "/join/<string>/<string>").methods(crow::HTTPMethod::GET)
//     ([&pool_ptr](const std::string& table_name_A, const std::string& table_name_B)
//     {
//         mysqlpp::ScopedConnection sc(pool_ptr, true);
//         MySqlStringRepositoryImpl concrete(sc);
//         IStringRepository& repo = concrete; 
//         bool result = repo.join(table_name_A, table_name_B);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }

// // ORDER
// template <typename... Middlewares>
// void thread_safe_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
// {
//     CROW_ROUTE(app, "/order/<string>/<string>/<string>").methods(crow::HTTPMethod::GET)
//     ([&pool_ptr](const std::string& table_name, const std::string& column, const std::string& order)
//     {
//         mysqlpp::ScopedConnection sc(pool_ptr, true);
//         MySqlStringRepositoryImpl concrete(sc);
//         IStringRepository& repo = concrete; 
//         bool result = repo.order(table_name, column, order);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }



// // INSERT 
// template <typename... Middlewares>
// void thread_safe_crow_insert_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
// {
//     CROW_ROUTE(app, "/insert/<string>").methods(crow::HTTPMethod::POST)
//     ([&pool_ptr](const crow::request& req, const std::string& table_name)
//     {
//         mysqlpp::ScopedConnection sc(pool_ptr, true);
//         MySqlStringRepositoryImpl concrete(sc);
//         IStringRepository& repo = concrete; 
//         crow::json::rvalue data = crow::json::load(req.body);
//         if (!data)
//         {
//             return crow::response(400, "Invalid JSON - expected list of objects");
//         } 
//         const std::string& keys = create_key_string(data[0]);
//         const std::string& values = create_values_string(data);
//         bool result = repo.insert(table_name, keys, values);
//         if (result)
//         {
//             return crow::response(500, repo.error());
//         }
//         return crow::response(201, "Entity created");
//     });
// }

// // UPDATE
// template <typename... Middlewares>
// void thread_safe_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
// {
//     CROW_ROUTE(app, "/update/<string>/<int>").methods(crow::HTTPMethod::PUT)
//     ([&pool_ptr](const crow::request& req, const std::string& table_name, int id)
//     {
//         mysqlpp::ScopedConnection sc(pool_ptr, true);
//         MySqlStringRepositoryImpl concrete(sc);
//         IStringRepository& repo = concrete; 
//         crow::json::rvalue data = crow::json::load(req.body);
//         if (!data)
//         {
//             return crow::response(400, "Missing or invalid JSON");
//         } 
//         std::string fields_string = create_update_fields_string(data);
//         bool result = repo.update(table_name, fields_string, id);
//         if (result)
//         {
//             return crow::response(500, repo.error());
//         }
//         return crow::response(201, "Entity updated");
//     });
// }

// // DELETE
// template <typename... Middlewares>
// void  thread_safe_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
// {
//     CROW_ROUTE(app, "/delete/<string>/<int>").methods(crow::HTTPMethod::DELETE)
//     ([&pool_ptr](const std::string& table_name, int id)
//     {
//         mysqlpp::ScopedConnection sc(pool_ptr, true);
//         MySqlStringRepositoryImpl concrete(sc);
//         IStringRepository& repo = concrete; 
//         bool result = repo.delete__(table_name, id);
//         if (result)
//         {
//             return crow::response(500, repo.error());
//         }
//         return crow::response(201, "Entity deleted");
//     });
// }

// // simple routes 

// template <typename... Middlewares>
// void simple_crow_get_all_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
// {
//     CROW_ROUTE(app, "/read/<string>").methods(crow::HTTPMethod::GET)
//     ([&mysql](const std::string& key)
//     {
//         MySqlStringRepositoryImpl concrete(mysql);
//         IStringRepository& repo = concrete; 
//         bool result = repo.select_all(key);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }

// template <typename... Middlewares>
// void simple_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
// {
//     CROW_ROUTE(app, "/read/<string>/<int>").methods(crow::HTTPMethod::GET)
//     ([&mysql](const std::string& key, int id)
//     {
//         MySqlStringRepositoryImpl concrete(mysql);
//         IStringRepository& repo = concrete; 
//         bool result = repo.select_by_id(key, id);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }

// // JOIN
// template <typename... Middlewares>
// void simple_crow_get_joined_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
// {
//     CROW_ROUTE(app, "/join/<string>/<string>").methods(crow::HTTPMethod::GET)
//     ([&mysql](const std::string& table_name_A, const std::string& table_name_B)
//     {
//         MySqlStringRepositoryImpl concrete(mysql);
//         IStringRepository& repo = concrete; 
//         bool result = repo.join(table_name_A, table_name_B);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }

// // ORDER
// template <typename... Middlewares>
// void simple_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
// {
//     CROW_ROUTE(app, "/order/<string>/<string>/<string>").methods(crow::HTTPMethod::GET)
//     ([&mysql](const std::string& table_name, const std::string& column, const std::string& order)
//     {
//         MySqlStringRepositoryImpl concrete(mysql);
//         IStringRepository& repo = concrete; 
//         bool result = repo.order(table_name, column, order);
//         if (!result)
//         { 
//             const std::vector<mysqlpp::Row>& rows = repo.get_rows();
//             const std::vector<std::string>& names = repo.get_names();
//             crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
//             return crow::response(200, res);
//         }
//         else
//         {
//             return crow::response(404, "Entity not found");
//         }
//     });
// }


// // INSERT 
// template <typename... Middlewares>
// void simple_crow_insert_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
// {
//     CROW_ROUTE(app, "/insert/<string>").methods(crow::HTTPMethod::POST)
//     ([&mysql](const crow::request& req, const std::string& table_name)
//     {
//         MySqlStringRepositoryImpl concrete(mysql);
//         IStringRepository& repo = concrete; 
//         crow::json::rvalue data = crow::json::load(req.body);
//         if (!data)
//         {
//             return crow::response(400, "Invalid JSON - expected list of objects");
//         } 
//         const std::string& keys = create_key_string(data[0]);
//         const std::string& values = create_values_string(data);
//         bool result = repo.insert(table_name, keys, values);
//         if (result)
//         {
//             return crow::response(500, repo.error());
//         }
//         return crow::response(201, "Entity created");
//     });
// }

// // UPDATE
// template <typename... Middlewares>
// void simple_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
// {
//     CROW_ROUTE(app, "/update/<string>/<int>").methods(crow::HTTPMethod::PUT)
//     ([&mysql](const crow::request& req, const std::string& table_name, int id)
//     {
//         MySqlStringRepositoryImpl concrete(mysql);
//         IStringRepository& repo = concrete; 
//         crow::json::rvalue data = crow::json::load(req.body);
//         if (!data)
//         {
//             return crow::response(400, "Missing or invalid JSON");
//         } 
//         std::string fields_string = create_update_fields_string(data);
//         bool result = repo.update(table_name, fields_string, id);
//         if (result)
//         {
//             return crow::response(500, repo.error());
//         }
//         return crow::response(201, "Entity updated");
//     });
// }

// // DELETE
// template <typename... Middlewares>
// void  simple_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
// {
//     CROW_ROUTE(app, "/delete/<string>/<int>").methods(crow::HTTPMethod::DELETE)
//     ([&mysql](const std::string& table_name, int id)
//     {
//         MySqlStringRepositoryImpl concrete(mysql);
//         IStringRepository& repo = concrete; 
//         bool result = repo.delete__(table_name, id);
//         if (result)
//         {
//             return crow::response(500, repo.error());
//         }
//         return crow::response(201, "Entity deleted");
//     });
// }

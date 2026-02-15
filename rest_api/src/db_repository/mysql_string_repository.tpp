#ifndef DB_STRING_REPOSITORY_TPP_
#define DB_STRING_REPOSITORY_TPP_



// thread safe routes
template <typename... Middlewares>
void thread_safe_crow_get_all_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    CROW_ROUTE(app, "/read/<string>").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](const std::string& key)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        MySqlStringRepositoryImpl concrete(sc);
        IStringRepository& repo = concrete; 
        bool result = repo.select_all(key);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}


template <typename... Middlewares>
void thread_safe_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    CROW_ROUTE(app, "/read/<string>/<int>").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](const std::string& key, int id)
    {
         
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        MySqlStringRepositoryImpl concrete(sc);
        IStringRepository& repo = concrete; 
        bool result = repo.select_by_id(key, id);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}

// JOIN
template <typename... Middlewares>
void thread_safe_crow_get_joined_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    CROW_ROUTE(app, "/join/<string>/<string>").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](const std::string& table_name_A, const std::string& table_name_B)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        MySqlStringRepositoryImpl concrete(sc);
        IStringRepository& repo = concrete; 
        bool result = repo.join(table_name_A, table_name_B);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}

// ORDER
template <typename... Middlewares>
void thread_safe_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    CROW_ROUTE(app, "/order/<string>/<string>/<string>").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](const std::string& table_name, const std::string& column, const std::string& order)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        MySqlStringRepositoryImpl concrete(sc);
        IStringRepository& repo = concrete; 
        bool result = repo.order(table_name, column, order);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}



// INSERT 
template <typename... Middlewares>
void thread_safe_crow_insert_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    CROW_ROUTE(app, "/insert/<string>").methods(crow::HTTPMethod::POST)
    ([&pool_ptr](const crow::request& req, const std::string& table_name)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        MySqlStringRepositoryImpl concrete(sc);
        IStringRepository& repo = concrete; 
        crow::json::rvalue data = crow::json::load(req.body);
        if (!data)
        {
            return crow::response(400, "Invalid JSON - expected list of objects");
        } 
        const std::string& keys = create_key_string(data[0]);
        const std::string& values = create_values_string(data);
        bool result = repo.insert(table_name, keys, values);
        if (result)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity created");
    });
}

// UPDATE
template <typename... Middlewares>
void thread_safe_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    CROW_ROUTE(app, "/update/<string>/<int>").methods(crow::HTTPMethod::PUT)
    ([&pool_ptr](const crow::request& req, const std::string& table_name, int id)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        MySqlStringRepositoryImpl concrete(sc);
        IStringRepository& repo = concrete; 
        crow::json::rvalue data = crow::json::load(req.body);
        if (!data)
        {
            return crow::response(400, "Missing or invalid JSON");
        } 
        std::string fields_string = create_update_fields_string(data);
        bool result = repo.update(table_name, fields_string, id);
        if (result)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity updated");
    });
}

// DELETE
template <typename... Middlewares>
void  thread_safe_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    CROW_ROUTE(app, "/delete/<string>/<int>").methods(crow::HTTPMethod::DELETE)
    ([&pool_ptr](const std::string& table_name, int id)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        MySqlStringRepositoryImpl concrete(sc);
        IStringRepository& repo = concrete; 
        bool result = repo.delete__(table_name, id);
        if (result)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity deleted");
    });
}

// simple routes 

template <typename... Middlewares>
void simple_crow_get_all_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
{
    CROW_ROUTE(app, "/read/<string>").methods(crow::HTTPMethod::GET)
    ([&mysql](const std::string& key)
    {
        MySqlStringRepositoryImpl concrete(mysql);
        IStringRepository& repo = concrete; 
        bool result = repo.select_all(key);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}

template <typename... Middlewares>
void simple_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
{
    CROW_ROUTE(app, "/read/<string>/<int>").methods(crow::HTTPMethod::GET)
    ([&mysql](const std::string& key, int id)
    {
        MySqlStringRepositoryImpl concrete(mysql);
        IStringRepository& repo = concrete; 
        bool result = repo.select_by_id(key, id);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}

// JOIN
template <typename... Middlewares>
void simple_crow_get_joined_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
{
    CROW_ROUTE(app, "/join/<string>/<string>").methods(crow::HTTPMethod::GET)
    ([&mysql](const std::string& table_name_A, const std::string& table_name_B)
    {
        MySqlStringRepositoryImpl concrete(mysql);
        IStringRepository& repo = concrete; 
        bool result = repo.join(table_name_A, table_name_B);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}

// ORDER
template <typename... Middlewares>
void simple_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
{
    CROW_ROUTE(app, "/order/<string>/<string>/<string>").methods(crow::HTTPMethod::GET)
    ([&mysql](const std::string& table_name, const std::string& column, const std::string& order)
    {
        MySqlStringRepositoryImpl concrete(mysql);
        IStringRepository& repo = concrete; 
        bool result = repo.order(table_name, column, order);
        if (!result)
        { 
            const std::vector<mysqlpp::Row>& rows = repo.get_rows();
            const std::vector<std::string>& names = repo.get_names();
            crow::json::wvalue res = mysql_helpers::to_crow_json(rows, names);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, "Entity not found");
        }
    });
}


// INSERT 
template <typename... Middlewares>
void simple_crow_insert_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
{
    CROW_ROUTE(app, "/insert/<string>").methods(crow::HTTPMethod::POST)
    ([&mysql](const crow::request& req, const std::string& table_name)
    {
        MySqlStringRepositoryImpl concrete(mysql);
        IStringRepository& repo = concrete; 
        crow::json::rvalue data = crow::json::load(req.body);
        if (!data)
        {
            return crow::response(400, "Invalid JSON - expected list of objects");
        } 
        const std::string& keys = create_key_string(data[0]);
        const std::string& values = create_values_string(data);
        bool result = repo.insert(table_name, keys, values);
        if (result)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity created");
    });
}

// UPDATE
template <typename... Middlewares>
void simple_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
{
    CROW_ROUTE(app, "/update/<string>/<int>").methods(crow::HTTPMethod::PUT)
    ([&mysql](const crow::request& req, const std::string& table_name, int id)
    {
        MySqlStringRepositoryImpl concrete(mysql);
        IStringRepository& repo = concrete; 
        crow::json::rvalue data = crow::json::load(req.body);
        if (!data)
        {
            return crow::response(400, "Missing or invalid JSON");
        } 
        std::string fields_string = create_update_fields_string(data);
        bool result = repo.update(table_name, fields_string, id);
        if (result)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity updated");
    });
}

// DELETE
template <typename... Middlewares>
void  simple_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql)
{
    CROW_ROUTE(app, "/delete/<string>/<int>").methods(crow::HTTPMethod::DELETE)
    ([&mysql](const std::string& table_name, int id)
    {
        MySqlStringRepositoryImpl concrete(mysql);
        IStringRepository& repo = concrete; 
        bool result = repo.delete__(table_name, id);
        if (result)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity deleted");
    });
}
#endif

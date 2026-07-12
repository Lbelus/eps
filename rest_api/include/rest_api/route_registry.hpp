#ifndef REST_API_ROUTE_REGISTRY_HPP
#define REST_API_ROUTE_REGISTRY_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <rest_api/mysql_conn_pool.hpp>

using RestApiMySqlRouteRegistrar =
    std::function<void(crow::App<crow::CORSHandler>&, SimpleConnectionPool&)>;

class RestApiMySqlRouteRegistry
{
public:
    static RestApiMySqlRouteRegistry& instance();

    bool add(const std::string& name, RestApiMySqlRouteRegistrar registrar);
    void bind(const std::string& name, crow::App<crow::CORSHandler>& app, SimpleConnectionPool& pool) const;
    bool contains(const std::string& name) const;
    std::vector<std::string> names() const;

private:
    std::unordered_map<std::string, RestApiMySqlRouteRegistrar> routes_;
};

#define REST_API_DETAIL_CONCAT_INNER(a, b) a##b
#define REST_API_DETAIL_CONCAT(a, b) REST_API_DETAIL_CONCAT_INNER(a, b)

#define REST_API_REGISTER_MYSQL_ROUTE(route_func)                                    \
    namespace                                                                        \
    {                                                                                \
        const bool REST_API_DETAIL_CONCAT(route_func, _rest_api_mysql_registered) =  \
            RestApiMySqlRouteRegistry::instance().add(                               \
                #route_func,                                                        \
                [](crow::App<crow::CORSHandler>& app, SimpleConnectionPool& pool)    \
                {                                                                    \
                    route_func(app, pool);                                           \
                }                                                                    \
            );                                                                       \
    }

#endif

#include <rest_api/route_registry.hpp>

#include <stdexcept>
#include <utility>

RestApiMySqlRouteRegistry& RestApiMySqlRouteRegistry::instance()
{
    static RestApiMySqlRouteRegistry registry;
    return registry;
}

bool RestApiMySqlRouteRegistry::add(
    const std::string& name,
    RestApiMySqlRouteRegistrar registrar
)
{
    if (name.empty() || !registrar)
    {
        return false;
    }

    routes_[name] = std::move(registrar);
    return true;
}

void RestApiMySqlRouteRegistry::bind(
    const std::string& name,
    crow::App<crow::CORSHandler>& app,
    SimpleConnectionPool& pool
) const
{
    const auto route = routes_.find(name);
    if (route == routes_.end())
    {
        throw std::runtime_error("Unknown MySQL route: " + name);
    }

    route->second(app, pool);
}

bool RestApiMySqlRouteRegistry::contains(const std::string& name) const
{
    return routes_.find(name) != routes_.end();
}

std::vector<std::string> RestApiMySqlRouteRegistry::names() const
{
    std::vector<std::string> output;
    output.reserve(routes_.size());

    for (const auto& route : routes_)
    {
        output.push_back(route.first);
    }

    return output;
}

#ifndef CONNECTION_FACTORY_HPP
#define CONNECTION_FACTORY_HPP

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include <rest_api/config_loader.hpp>
#include <rest_api/mysql_conn_pool.hpp>

class IConnectionPool
{
public:
    using Id = std::uint64_t;

    IConnectionPool()
        : id_(next_id_++)
    {}

    virtual ~IConnectionPool() = default;

    Id id() const
    {
        return id_;
    }

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

private:
    Id id_;
    static inline Id next_id_ = 1;
};

class MySqlConnectionPool : public IConnectionPool
{
public:
    explicit MySqlConnectionPool(const db_config_t& config)
        : config_(config),
          mysql_config_{
              config.database,
              config.host,
              config.user,
              config.password,
              config.port
          }
    {}

    bool connect() override
    {
        if (!mysqlpp::Connection::thread_aware())
        {
            std::cerr << "MySQL++/libmysqlclient not built thread-aware on this system\n";
            return false;
        }

        pool_ = std::make_unique<SimpleConnectionPool>(mysql_config_);
        return true;
    }

    void disconnect() override
    {
        pool_.reset();
    }

    bool is_connected() const override
    {
        return static_cast<bool>(pool_);
    }

    SimpleConnectionPool& pool()
    {
        if (!pool_)
        {
            throw std::runtime_error("MySQL connection pool is not connected");
        }

        return *pool_;
    }

    const db_config_t& config() const
    {
        return config_;
    }

private:
    db_config_t config_;
    mysql_pool_config_t mysql_config_;
    std::unique_ptr<SimpleConnectionPool> pool_;
};

class ConnectionPoolFactory
{
public:
    static std::unique_ptr<IConnectionPool> create(const db_config_t& config)
    {
        switch (config.type)
        {
            case DbType::MySQL:
                return std::make_unique<MySqlConnectionPool>(config);

            case DbType::Redis:
                throw std::runtime_error("Redis connection pools are not implemented yet");

            case DbType::PostgreSQL:
                throw std::runtime_error("PostgreSQL connection pools are not implemented yet");

            case DbType::unknown:
                break;
        }

        throw std::runtime_error("Unknown database type for connection pool");
    }
};

class ConnectionPoolRegistry
{
public:
    using Id = IConnectionPool::Id;
    void update_route_map(Id id, const std::vector<std::string>& routes)
    {
        for (const auto& route : routes)
        {
            route_map_[route] = id;
        }
    }

    Id add(std::unique_ptr<IConnectionPool> pool, const db_config_t& config)
    {
        if (!pool)
        {
            throw std::invalid_argument("Cannot register a null connection pool");
        }

        const Id id = pool->id();
        pools_.emplace_back(std::move(pool));
        update_route_map(id, config.route_vec);
        return id;
    }


    Id create_and_add(const db_config_t& config)
    {
        return add(ConnectionPoolFactory::create(config), config);
    }

    IConnectionPool& get(Id id)
    {
        auto it = find_pool(id);
        if (it == pools_.end())
        {
            throw std::runtime_error("Connection pool not found");
        }
        return **it;
    }

    bool remove(Id id)
    {
        auto it = find_pool(id);
        if (it == pools_.end())
        {
            return false;
        }

        pools_.erase(it);
        return true;
    }

    void clear()
    {
        pools_.clear();
        route_map_.clear();
    }

private:
    using PoolIterator = std::vector<std::unique_ptr<IConnectionPool>>::iterator;

    PoolIterator find_pool(Id id)
    {
        return std::find_if(
            pools_.begin(),
            pools_.end(),
            [id](const std::unique_ptr<IConnectionPool>& pool)
            {
                return pool->id() == id;
            }
        );
    }
    std::map<std::string, Id> route_map_;
    std::vector<std::unique_ptr<IConnectionPool>> pools_;
};

#endif

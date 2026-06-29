#ifndef CONNECTION_FACTORY_HPP
#define CONNECTION_FACTORY_HPP

#include <iostream>
#include <memory> 


// FACTORY DESIGN PATTERN WITH RAII

// per recommandation : Avoid overuse of raw new by making use of smart pointers and RAII principles 


class IConnectionPool
{
public:
    using Id = std::uint64_t;
    IConnection()
        : id_(next_id_++)
    {
        std::cout << "Connection object generated"
    }
    
    Id id() const
    {
        return id_;
    }
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual int is_connected() const = 0;
    virtual ~IConnection() = default;
    {
        std::cout << "Connection object destroyed"
    };
    private:
    Id id_;
    static inline Id next_id_ = 1;
};

class MySqlConnectionPool : public IConnectionPool
{
private:
    const db_config_t* config;
    std::unique_ptr<SimpleConnectionPool> pool_;

public:
    MySqlConnection(const db_config_t* config)
    : config(config) id_(next_id_++)
    {
        std::cout << "My sql connection pool configuration loaded" << std::endl;
    }

    int connect() override
    {
        SimpleConnectionPool pool(config);
        if (!mysqlpp::Connection::thread_aware())
        {
            std::cerr << "MySQL++/libmysqlclient not built thread-aware on this system\n";
            return EXIT_FAILURE;
        }
        pool_ = std::make_unique<SimpleConnectionPool>(config_);
        return EXIT_SUCCESS;
    }

    SimpleConnectionPool& pool()
    {
        if (!pool_)
            throw std::runtime_error("Pool not connected");

        return *pool_;
    }

    void disconnect() override
    {

    }

    bool is_connected() override
    {

    }
};

class ConnectionPoolFactory 
{
public:
    static std::unique_ptr<IConnection> create(const db_config_t config);
    {
        switch (config.type)
        {
            case DbType::MySQL:
            return std::make_unique<MySqlConnectionPool>(config); 
            
            case DbType::Redis: 
            return std::make_unique<RedisConnectionPool>(config);

            case DbType::PostgreSQL: 
            return std::make_unique<RedisConnectionPool>(config);
        }
    };
    ~ConnectionFactory();
};


class connectionPoolRegistry
{
private:
    std::vector<std::unique_ptr<IConnectionPool> pool_vec;        
public:
    using Id = std::uint64_t;

    void add_connection_pool()
    {
        pool_vec.emplace_back(std:move(pool_vec);
    }

    bool remove(Id id)
    {
        int index = 0;
        for (const auto& pool : pools)
        {
            if (pool.id() == id)
            {
                pools.erase(begin() + index);
                return true;
            }
            index += 1;
        }
        std::cout << "Could not find pool at index: " << id << std::endl;
    }

    


};


// class Destroyer : public Battleship
// {
// public:
//     Destroyer()
//     {
//         std::cout << "Destroyer Created" << std::endl;
//     }
//     void Fire() override
//     {
//         std::cout << "Destroyer Fire" << std::endl;
//     }
//     void Steer() override
//     {
//         std::cout << "Destroyer Steer" << std::endl;
//     }
// };

// class Carrier : public Battleship
// {
// public:
//     Carrier()
//     {
//         std::cout << "Carrier Created" << std::endl;
//     }
//     void Fire() override
//     {
//         std::cout << "Carrier Fire" << std::endl;
//     }
//     void Steer() override
//     {
//         std::cout << "Carrier Steer" << std::endl;
//     }
// };

// // run-time polymorphism

// class ShipCreator
// {
// public:
//     virtual std::unique_ptr<Battleship> FactoryMethod() = 0;
//     virtual ~ShipCreator() {}

//     std::unique_ptr<Battleship> CreateShip()
//     {
//         std::unique_ptr<Battleship> smart_ptr = this->FactoryMethod();
//         return smart_ptr;
//     }
// };

// class CarrierCreator : public ShipCreator
// {
//     std::unique_ptr<Battleship> FactoryMethod() override
//     {
//         return std::make_unique<Carrier>();
//     }
// };

// class DestroyerCreator : public ShipCreator
// {
//     std::unique_ptr<Battleship> FactoryMethod() override
//     {
//         return std::make_unique<Destroyer>();
//     }
// };

// int main()
// {
//     std::unique_ptr<ShipCreator> creator = std::make_unique<CarrierCreator>();
//     std::unique_ptr<Battleship> battleship1 = creator->CreateShip();
//     battleship1->Fire();
//     battleship1->Steer();

//     creator = std::make_unique<DestroyerCreator>();
//     std::unique_ptr<Battleship> battleship2 = creator->CreateShip();
//     battleship2->Fire();
//     battleship2->Steer();

//     return 0;
// }

// #endif

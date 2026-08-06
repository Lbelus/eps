#ifndef MY_REDIS_CLIENT_
#define MY_REDIS_CLIENT_

#include <string>
#include <hiredis/hiredis.h>
#include <string.h>
#include <memory>
#include <iostream>
#include <vector>

namespace redis_client
{
    #define INVALID     0
    #define TCP_ADDR    1
    #define IP_ADDR     2
    #define TCP_STR     "tcp://"

    class RedisClient
    {
    Threadpool Tp;
     vate:
      redisContext* context;
      std::string host;
      int port;

      int getAdressType(const std::string& address);
      void connection(const std::string& address);
      void connection_opt(const std::string& address);
      void splitHostAndPort(const std::string& input, std::string& host, int& port);
 
      std::string concatenate()
      {
          return "";
      }

      template<typename... Args>
      std::string concatenate(const std::string& first, Args... args)
            {
                return first + concatenate(args...);
            }

        public:
            RedisClient(const std::string& address)
            {
                if (getAdressType(address) == TCP_ADDR)
                {
                    connection_opt(address);
                }
                else if (getAdressType(address) == IP_ADDR)
                {
                    connection(address);
                }
            }

// KEY/VALUES
            std::string set(const char* key, const char* value);
            std::string get(const char* key);
            std::vector<std::string> keys(const char *pattern);
            std::string type(const char* key);
            std::string del(const char* key);
            std::string unlink(const char* key);
            std::string expire(const char* key, int seconds);
            std::string rename(const char *old_key, const char *new_key);
//LISTS
            std::string lpush(const char* key, const char* value);
            std::string rpush(const char* key, const char* value);
            std::string lpop(const char* key);
            std::string rpop(const char* key);
            std::string llen(const char* key);
            std::string lrem(const char* key, int count, const char *element);
            std::string lindex(const char* key, int index);
            std::string lset(const char* key, int index, const char* value);

//HASHES
            std::string hget(const char* key, const char* field);
            std::string hexists(const char* key, const char* field);
            std::string hmset(const char* key, const char** fields, const char** values, size_t fieldCount);
            std::string hdel(const char* key, const char* field);
            std::string hset(const char* key, const char* field, const char* value);
            std::vector<std::string> hvals(const char* key);
            std::vector<std::pair<std::string, std::string>> hgetall(const char* key);
            std::vector<std::string> hkeys(const char* key);
            std::string hlen(const char* key);

//common
            std::string echo(const char* message);
            std::string ping();
            std::string flushall();
            std::string info(const char* section = nullptr);

            ~RedisClient()
            {
                if (context)
                {   
                    redisFree(context);
                }
            }
    };
};


// unsigned int GetThreadCount(unsigned int divBy);
// using Task = std::function<void()>;

// class ThreadPool
// {
//     private:
//         std::vector<std::thread> workers;
//         std::queue<Task> tasks;
//         std::mutex mutex;
//         bool stop = false;

//     public:
//     ThreadPool(unsigned int nbThread)
//     {
//         unsigned int index = 0;
//         while (index < nbThread)
//         {
//             workers.emplace_back(&ThreadPool::workerFunction, this);
//             index += 1;
//         }
//     }


//     void enqueue(Task task) 
//     {
//         std::lock_guard<std::mutex> lock(mutex);
//         tasks.push(task);
//     }


//     void workerFunction()
//     {
//         while (!stop)
//         {
//             Task task;
//             {
//                 std::lock_guard<std::mutex> lock(mutex);
//                 if (tasks.empty()) 
//                 {
//                     continue;
//                 }
//                 task = tasks.front();
//                 tasks.pop();
//             }
//             task();
//         }
//     }
// };


// class clientMov: public my_redis::RedisClient
// {
// private:
//     redisContext* context;
//     std::string host;
//     int port;

// public:
//     clientMov(const std::string& address) : RedisClient(address), context(nullptr), host(""), port(0) {}

//     clientMov(clientMov&& other) noexcept : RedisClient(std::move(other)), context(other.context), host(std::move(other.host)), port(other.port) 
// {
//         other.context = nullptr;
//     }

//     clientMov& operator=(clientMov&& other) noexcept
//     {
//         if (this != &other) 
//         {
//             RedisClient::operator=(std::move(other));
//             
//             if (context)
//             {   
//                 redisFree(context);
//             }
//             context = other.context;
//             other.context = nullptr;
//             host = std::move(other.host);
//             port = other.port;
//         }
//         return *this;
//     }

//     ~clientMov() 
//     {
//         if (context)
//         {   
//             redisFree(context);
//         }
//     }
// };


// void redisTask(std::string)
// {
//     std::shared_ptr<redis_client::RedisClient> shared_redis_conn;
// }

class redis_conn_pool
{
private:
    using redis_client::RedisClient = Connection;
    using std::unique_ptr<Connection> = unique_conn_t;
    idle_time = 3;
    
    struct connection_info_s
    {
        unique_conn_t conn;
        time_t last_used;
        bool in_use;
        
        connection_info_t(unique_conn_t conn_) :
        conn(conn_),
        last_used(time(0)),
        in_use(true)
        {}
        bool operator<(const connection_info_s rhs) const
        {
            const connection_info_s& lhs = *this;
            if (lhs.in_use == rhs.in_use)
            {
                return lsh.last_used < rhs.last_used;
            }
            else
            {
                return lhs.in_use;
            }     
        }
    };
    typedef connection_info_s ConnectionInfo;
    typedef std::list<ConnectionInfo> poolList;
    typedef poolList::iterator poolIt

public:
    redis_conn_pool(const std::string& address) :
    {
        unsigned int count = 0;
        while (count < min_con)
        {
            unique_conn_t redis_client::RedisClient rc(address);
            conn_vec.push_back(rc);
            count += 1;
        }
    }
    std::unique_ptr<redis_client::RedisClient> acquire()
    {
        conns_in_use += 1;
        return conn_vec.pop_back();
    }
    unique_conn_t exchange();
    unique_conn_t grab();
    void release();
    void shrink();
    void destroy();
    unsigned int max_idle_time() = 0;
    size_t size() const
    {
        return pool_.size();
    }

    void clear()
    {
// here lies some lock to prevent the app to use a conn_ at the same time. 
        for (std::vector<unique_conn_t>::iterator it = conn_vec.begin(); it != conn_vec.end();)
        {
            conn_vec.erase(it);
            ++it;
        } 
    }

    ~redis_conn_pool()
    {
        for (std::vector<unique_conn_t>::iterator it = conn_vec.begin(); it != conn_vec.end();)
        {
            conn_vec.erase(it);
            ++it;
        } 
    }
};


#endif

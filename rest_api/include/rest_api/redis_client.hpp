#ifndef MY_REDIS_CPOOL_H
#define MY_REDIS_CPOOL_H

#include <string>
#include <hiredis/hiredis.h>
#include <string.h>
#include <memory>
#include <iostream>
#include <vector>
#include <mysql++/beemutex.h>
#include <list>
#include <assert.h>
#include <ctime>
#include <algorithm>

namespace redis_client
{
    #define INVALID     0
    #define TCP_ADDR    1
    #define IP_ADDR     2
    #define TCP_STR     "tcp://"

    class RedisClient
    {
      redisContext* context;
      std::string host;
      int port;

      int   getAdressType(const std::string& address);
      void  connection(const std::string& address);
      void  connection_opt(const std::string& address);
      void  splitHostAndPort(const std::string& input, std::string& host, int& port);
 
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

template <typename ConnInfoT>
class TooOld
{
public:
    using argument_type = ConnInfoT;
    using result_type = bool;

    explicit TooOld(unsigned int tmax)
    : min_age_(std::time(nullptr) - tmax)
    {
    }

    bool operator()(const ConnInfoT& conn_info) const
    {
        return !conn_info.in_use &&
        conn_info.last_used <= min_age_;
    }

private:
    std::time_t min_age_;
};


class redis_conn_pool
{
public:
    using Connection = redis_client::RedisClient;
    using unique_conn_t = std::unique_ptr<Connection>;

private:
    
    struct connection_info_s
    {
        unique_conn_t conn;
        std::time_t last_used;
        bool in_use;
        
        connection_info_s(Connection* conn_) :
        conn(conn_),
        last_used(time(0)),
        in_use(true)
        {}
        bool operator<(const connection_info_s& rhs) const
        {
            const connection_info_s& lhs = *this;
            if (lhs.in_use == rhs.in_use)
            {
                return lhs.last_used < rhs.last_used;
            }
            else
            {
                return lhs.in_use;
            }     
        }
    };
    typedef connection_info_s ConnectionInfo;
    typedef std::list<ConnectionInfo> PoolList;
    typedef PoolList::iterator PoolIt;

    Connection* find_mru()
    {
        PoolIt mru = std::max_element(pool_.begin(), pool_.end());
        if (mru != pool_.end() && !mru->in_use)
        {
            mru->in_use = true;
            return mru->conn.get();
        }
        else
        {
            return nullptr;
        }
    }

    void remove(const PoolIt& it)
    {
        it->conn.reset();
        pool_.erase(it);
    }


    void remove_old_connections()
    {
	    TooOld<ConnectionInfo> too_old(max_idle_time());

	    PoolIt it = pool_.begin();
	    while ((it = std::find_if(it, pool_.end(), too_old)) != pool_.end())
        {
		    remove(it++);
	    }
    } 

    PoolList pool_;
    mysqlpp::BeecryptMutex mutex_;

public:
    redis_conn_pool(){}
    virtual ~redis_conn_pool()
    {
        assert(empty());
    }

    bool empty() const
    {
        return pool_.empty();
    }

    virtual Connection* exchange(const Connection* pool_conn)
    {
        remove(pool_conn);
        return grab();
    }

    virtual Connection* grab()
    {
        mysqlpp::ScopedLock lock(mutex_);
        remove_old_connections();
        if (Connection* mru = find_mru()) 
        {
            return mru;
        }
        else
        {
            pool_.push_back(ConnectionInfo(create()));
            return pool_.back().conn.get();
        }
        return nullptr;
    }

    virtual void release(const Connection* pool_conn)
    {
        mysqlpp::ScopedLock lock(mutex_);
        for (PoolIt it = pool_.begin(); it != pool_.end(); ++it)
        {
            if (it->conn.get() == pool_conn)
            {
                it->in_use = false;
                it->last_used = time(0);
                break;
            }
        }
    }

    void remove(const Connection* pool_conn)
    {
        mysqlpp::ScopedLock lock(mutex_);
        for (PoolIt it = pool_.begin(); it != pool_.end(); ++it)
        {
            if (it->conn.get() == pool_conn)
            {
                remove(it);
                return;
            }
        } 
    }
    virtual Connection* safe_grab()
    {
        Connection* pool_conn;
        while (!(pool_conn = grab())->ping().c_str())
        {
            remove(pool_conn);
            pool_conn = nullptr;
        }
        return pool_conn;
    }
    void shrink()
    {
        clear(false);
    }

protected:

    void clear(bool all = true)
    {
        mysqlpp::ScopedLock lock(mutex_);
        PoolIt it = pool_.begin();
        while (it != pool_.end())
        {
            if (all == true || !it->in_use)
            {
                remove(it++);
            }
            else
            {
                ++it;
            }
        }
    }
    virtual Connection* create() = 0;
    virtual void destroy(Connection*) = 0;
    virtual unsigned int max_idle_time() = 0;
    size_t size() const
    {
        return pool_.size();
    }
};

class RedisScopedConnection {
public:
  explicit RedisScopedConnection(redis_conn_pool& pool)
      : pool_(&pool), conn_(pool.grab())
  {}

  ~RedisScopedConnection()
  {
      if (pool_ && conn_) {
          pool_->release(conn_);
      }
  }

  RedisScopedConnection(const RedisScopedConnection&) = delete;
  RedisScopedConnection& operator=(const RedisScopedConnection&) = delete;

  RedisScopedConnection(RedisScopedConnection&& other) noexcept
      : pool_(other.pool_), conn_(other.conn_)
  {
      other.pool_ = nullptr;
      other.conn_ = nullptr;
  }

  RedisScopedConnection& operator=(RedisScopedConnection&& other) noexcept
  {
      if (this != &other) {
          if (pool_ && conn_) {
              pool_->release(conn_);
          }
          pool_ = other.pool_;
          conn_ = other.conn_;
          other.pool_ = nullptr;
          other.conn_ = nullptr;
      }
      return *this;
  }

  redis_conn_pool::Connection* operator->() { return conn_; }
  redis_conn_pool::Connection& operator*() { return *conn_; }

private:
  redis_conn_pool* pool_;
  redis_conn_pool::Connection* conn_;
};

#endif

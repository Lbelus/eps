#ifndef MYSQL_CONN_POOL_HPP
#define MYSQL_CONN_POOL_HPP

#include <mysql++/mysql++.h>
#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>
#include <string>
// #include <mysql_helpers.hpp>
#include <crow/middlewares/cors.h>
#include <crow.h>

#ifndef CONNECTION_STRUCT
#define CONNECTION_STRUCT
struct mysql_connection_s
{
    const char* db;
    const char* server;
    const char* user;   
    const char* password;
    unsigned int port;
};
typedef struct mysql_connection_s mysql_connection_t;
#endif

struct mysql_pool_config_t
{
    std::string db;
    std::string server;
    std::string user;
    std::string password;
    unsigned int port;
};


unsigned int GetThreadCount(unsigned int divBy);

// ConnectionPool has three methods that you need to override in a subclass to make it concrete: 
// - create(), 
// - destroy()
// - max_idle_time(). 

// These overrides let the base class delegate operations it can’t successfully do itself to its subclass.
namespace mysqlpp {
#if !defined(DOXYGEN_IGNORE)
// Make Doxygen ignore this
class MYSQLPP_EXPORT Connection;
#endif


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


class MYSQLPP_EXPORT MySqlConnectionPool
{
public:
    // using Connection = redis_client::MySqlClient;
    using ConnectionPtr = std::unique_ptr<Connection>;

    MySqlConnectionPool(){}
    virtual ~MySqlConnectionPool()
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
        while (!(pool_conn = grab())->ping())
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

private:
    
    struct connection_info_s
    {
        ConnectionPtr conn;
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

    using ConnectionInfo = connection_info_s;
    using PoolList = std::list<ConnectionInfo>;
    using PoolIt = PoolList::iterator;

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
};

class MYSQLPP_EXPORT MySqlScopedConnection
{
public:
  explicit MySqlScopedConnection(MySqlConnectionPool& pool)
      : pool_(&pool), conn_(pool.grab())
  {}

  ~MySqlScopedConnection()
  {
      if (pool_ && conn_) {
          pool_->release(conn_);
      }
  }

  MySqlScopedConnection(const MySqlScopedConnection&) = delete;
  MySqlScopedConnection& operator=(const MySqlScopedConnection&) = delete;

  MySqlScopedConnection(MySqlScopedConnection&& other) noexcept
      : pool_(other.pool_), conn_(other.conn_)
  {
      other.pool_ = nullptr;
      other.conn_ = nullptr;
  }

  MySqlScopedConnection& operator=(MySqlScopedConnection&& other) noexcept
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

  Connection* operator->() { return conn_; }
  Connection& operator*() { return *conn_; }

private:
  MySqlConnectionPool* pool_;
  Connection* conn_;
};

}


class SimpleConnectionPool : public mysqlpp::MySqlConnectionPool
{

public:
	// The object's only constructor
	explicit SimpleConnectionPool(const mysql_connection_t* conn_id)
        : conns_in_use_(0),
	      db_(conn_id->db),
	      server_(conn_id->server),
	      user_(conn_id->user),
	      password_(conn_id->password),
          port_(conn_id->port)
	{}

    explicit SimpleConnectionPool(const mysql_pool_config_t& config)
        : conns_in_use_(0),
          db_(config.db),
          server_(config.server),
          user_(config.user),
          password_(config.password),
          port_(config.port)
    {}

	// The destructor.  We _must_ call ConnectionPool::clear() here,
	// because our superclass can't do it for us.
	~SimpleConnectionPool()
	{
		clear();
	}
    
    mysqlpp::Connection* grab() override
    {
        ++conns_in_use_;
        return mysqlpp::MySqlConnectionPool::grab();
    }
    mysqlpp::Connection* safe_grab() override
    {
        ++conns_in_use_;
        return mysqlpp::MySqlConnectionPool::safe_grab();
    }
    void release(const mysqlpp::Connection* pc) override
    {
        mysqlpp::MySqlConnectionPool::release(pc);
        --conns_in_use_;
    }
protected:
    // Superclass overrides
    mysqlpp::Connection* create() override
    {
        // Create connection using the parameters we were passed upon
        // creation.  This could be something much more complex, but for
        // the purposes of the example, this suffices.
        std::cout.put('C'); std::cout.flush(); // indicate connection creation
        return new mysqlpp::Connection(
                db_.empty() ? 0 : db_.c_str(),
                server_.empty() ? 0 : server_.c_str(),
                user_.empty() ? 0 : user_.c_str(),
                password_.empty() ? "" : password_.c_str(),
                port_);
    }

    void destroy(mysqlpp::Connection* cp) override
    {
        // Our superclass can't know how we created the Connection, so
        // it delegates destruction to us, to be safe.
        std::cout.put('D'); std::cout.flush(); // indicate connection destruction
        delete cp;
    }

    unsigned int max_idle_time() override
    {
        // Set our idle time at an example-friendly 3 seconds.  A real
        // pool would return some fraction of the server's connection
        // idle timeout instead.
        return 3;
    }
private:

    unsigned int conns_in_use_;
    std::string db_;
    std::string server_;
    std::string user_;
    std::string password_;
    unsigned int port_ = 0;
};


template <typename... Middlewares>
using mysql_simple_func_ptr_t = void (*)(crow::Crow<Middlewares...>&, mysqlpp::Connection&);

template <typename... Middlewares>
using mysql_thread_safe_func_ptr_t = void (*)(crow::Crow<Middlewares...>&, SimpleConnectionPool&);

template <typename... Middlewares>
using mysql_thread_safe_cors_func_ptr_t = void (*)(crow::App<crow::CORSHandler>&, SimpleConnectionPool&);


// typedef void (*mysql_simple_func_ptr_t) (crow::Crow<Middlewares...>&, mysqlpp::Connection&);
// typedef void (*mysql_thread_safe_func_ptr_t) (crow::Crow<Middlewares...>&, SimpleConnectionPool&);

// usage:
        // poolptr = new SimpleConnectionPool(cmdline);
        // try {
        // mysqlpp::ScopedConnection cp(*poolptr, true);
        // if (!cp->thread_aware()) {
        //     cerr << "MySQL++ wasn't built with thread awareness!  " <<
        //             argv[0] << " can't run without it." << endl;
        //     return 1;
        // }

#endif 

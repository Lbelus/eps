#ifndef MYSQL_CONN_POOL_HPP
#define MYSQL_CONN_POOL_HPP

#include <mysql++/mysql++.h>
#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>
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


unsigned int GetThreadCount(unsigned int divBy);

// ConnectionPool has three methods that you need to override in a subclass to make it concrete: 
// - create(), 
// - destroy()
// - max_idle_time(). 

// These overrides let the base class delegate operations it can’t successfully do itself to its subclass.

class SimpleConnectionPool : public mysqlpp::ConnectionPool
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
	// The destructor.  We _must_ call ConnectionPool::clear() here,
	// because our superclass can't do it for us.
	~SimpleConnectionPool()
	{
		clear();
	}
    
    mysqlpp::Connection* grab() override
    {
        ++conns_in_use_;
        return mysqlpp::ConnectionPool::grab();
    }
    mysqlpp::Connection* safe_grab() override
    {
        ++conns_in_use_;
        return mysqlpp::ConnectionPool::safe_grab();
    }
    void release(const mysqlpp::Connection* pc) override
    {
        mysqlpp::ConnectionPool::release(pc);
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

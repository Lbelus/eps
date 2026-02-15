#ifndef DB_STRING_REPOSITORY_HPP_
#define DB_STRING_REPOSITORY_HPP_

#include <mysql++/mysql++.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <mysql_helpers.hpp>
#include <mysql_conn_pool.hpp>

#if !REPO_FAKE_ONLY
#include <crow.h>
#endif

using namespace mysql_helpers;

struct IStringRepository
{
    virtual ~IStringRepository() = default;
    virtual void store_names(const mysqlpp::StoreQueryResult& query_result) = 0;
    //C
    virtual bool insert(const std::string& table_name,const std::string& keys, const std::string& values) = 0;
    //R
    virtual bool select_all(const std::string& key) = 0;
    virtual bool select_by_id(const std::string& key, int id) = 0;
    virtual bool join(const std::string& table_name_A, const std::string& table_name_B) = 0;
    virtual bool order(const std::string& table_name, const std::string& column, const std::string& order) = 0;
    //U
    virtual bool update(const std::string& table_name, const std::string& fields_string, int id) = 0;
    //D
    virtual bool delete__(const std::string& table_name, int id) = 0;
    // getter
    virtual const char* error() = 0;
    virtual const std::vector<mysqlpp::Row>& get_rows() = 0;
    virtual const std::vector<std::string>& get_names() = 0;
};


// Client-Side Validation: Perform basic checks (e.g., required fields, formats) to improve user experience.
// Server-Side Validation: Always validate inputs on the server to prevent malicious data from bypassing client-side checks.

#if !REPO_FAKE_ONLY
class MySqlStringRepository : public IStringRepository
{
    private:
        mysqlpp::SimpleResult                       simple_result;
        std::reference_wrapper<mysqlpp::Connection> conn_;
        const char*                                 error_msg; 
        std::vector<mysqlpp::Row>                   rows;
        std::vector<std::string>                    names;
        mysqlpp::Connection& conn()
        {
            return conn_.get();
        }
        void store_names(const mysqlpp::StoreQueryResult& query_result);
    public:
    // Immediate syntax validation of input data at initilialization level. 
    // flag things that are definitely wrong; e.g. missing required fields, type mismatch, unparsable strings, any attempts of code injection and the presence (or lack of) of security tokens.
    
    // CTOR overload, allow us to use both a simple conn and scoped one (RAII). both are returning the same type so no further handling is required in the class.  
    explicit MySqlStringRepository(mysqlpp::Connection& conn)
        : conn_(conn) {
        std::cout << "PLACEHOLDER SQL: Server-side validation phase 1\n";
    }

    // Construct from a ScopedConnection (RAII wrapper around Connection)
    explicit MySqlStringRepository(mysqlpp::ScopedConnection& scoped)
        : conn_(*scoped) { // deref to underlying mysqlpp::Connection
        std::cout << "PLACEHOLDER SQL: Server-side validation phase 1\n";
    }

    // Business-logic validation at method level
    // When this validation passes, the input data is at least syntactically correct and may be passed on to the services, where a more strict validation occurs; i.e. does the input data make sense business-wise, does the resource with that ID exist - and so on.
        //C
        bool insert(const std::string& table_name,const std::string& keys, const std::string& values) override;
        //R
        bool select_all(const std::string& key) override;
        bool select_by_id(const std::string& key, int id) override;
        bool join(const std::string& table_name_A, const std::string& table_name_B) override;
        bool order(const std::string& table_name, const std::string& column, const std::string& order) override;
        //U
        bool update(const std::string& table_name, const std::string& fields_string, int id) override;
        //D
        bool delete__(const std::string& table_name, int id) override;
        // getter
        const char* error() override;
        const std::vector<mysqlpp::Row>& get_rows() override;
        const std::vector<std::string>& get_names() override;
};
#endif

#if REPO_ENABLE_FAKE
// define fake repo here
#endif

#if REPO_ENABLE_FAKE
// using MySqlStringRepositoryImpl = MySqlFakeStringRepository;
#else
using MySqlStringRepositoryImpl = MySqlStringRepository;
#endif

#if !REPO_FAKE_ONLY

// thread safe routes
template <typename... Middlewares>
void thread_safe_crow_get_all_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);
template <typename... Middlewares>
void thread_safe_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);

// JOIN
template <typename... Middlewares>
void thread_safe_crow_get_joined_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);

// ORDER
template <typename... Middlewares>
void thread_safe_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);

// INSERT 
template <typename... Middlewares>
void thread_safe_crow_insert_entity(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);

// UPDATE
template <typename... Middlewares>
void thread_safe_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);

// DELETE
template <typename... Middlewares>
void  thread_safe_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr);

// simple routes 

template <typename... Middlewares>
void simple_crow_get_all_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);
template <typename... Middlewares>
void simple_crow_get_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

// JOIN
template <typename... Middlewares>
void simple_crow_get_joined_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

// ORDER
template <typename... Middlewares>
void simple_crow_get_ordered_entities(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

// INSERT 
template <typename... Middlewares>
void simple_crow_insert_entity(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

// UPDATE
template <typename... Middlewares>
void simple_crow_update_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

// DELETE
template <typename... Middlewares>
void  simple_crow_delete_entity_by_id(crow::Crow<Middlewares...>& app, mysqlpp::Connection& mysql);

#include <mysql_string_repository.tpp>

#endif

#endif

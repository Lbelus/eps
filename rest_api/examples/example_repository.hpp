#ifndef DB_EXAMPLE_REPOSITORY_HPP
#define DB_EXAMPLE_REPOSITORY_HPP

#if !REPO_FAKE_ONLY
#include <crow.h>
#endif
#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>
#include <vector>
#include <string>
#include <mysql_conn_pool.hpp>

/*
 * GENERAL INFORMATION: 
 * 
 * The overall structure of this rest API relies heavily on SSQLS;
 * 
 * You should READ THE DOC regarding Specialized SQL Structures:
 *
 * https://tangentsoft.com/mysqlpp/doc/html/userman/ssqls.html
*/



/*  
 * STEP 1: DEFINE the models with SSQLS (example_users)
 * src: https://tangentsoft.com/mysqlpp/doc/html/userman/ssqls.html
 *
 * Macro general format: sql_create_#(NAME, COMPCOUNT, SETCOUNT, TYPE1, ITEM1, ... TYPE#, ITEM#)
 * # is the number of member variables,
 * NAME is the name of the structure
 *   TYPEx = the type of a member variable
 *   ITEMx = the variable’s name
 *   COMPCOUNT = how many fields are used to auto-generate comparison operators (==, <, etc.)
 *   SETCOUNT = how many leading fields you want to initialize via a generated ctor & set() method
 * 
 * let's implement a sql table with SSQLS:
 *
 * CREATE TABLE example_users (
 *   id INT AUTO_INCREMENT PRIMARY KEY,
 *      name VARCHAR(100) NOT NULL,
 *      email VARCHAR(255) NOT NULL UNIQUE,
 *      created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP 
 *   );
*/

sql_create_4(ExampleUser, 1, 4,
    mysqlpp::sql_int,       id,
    mysqlpp::sql_varchar,   name,
    mysqlpp::sql_varchar,   email,
    mysqlpp::sql_datetime,  created_at
)

// OPTIONAL: You can add a convenience ctor
// static ExampleUser make_new_example_user(const std::string& name, const std::string& email)
// {
//     ExampleUser ex_user;
//     ex_user.id = 0;
//     ex_user.name = name;
//     ex_user.email = email;
//     ex_user.created_at = mysqlpp::DateTime(); // will be filled by db on insert
//     return ex_user;
// }

// ==============================================


/* 
* STEP 2: Repository interface (exampleUsers)
*
*/

struct IExampleUsersRepository
{
    virtual ~IExampleUsersRepository() = default;

    virtual int get_by_id(int id) = 0;
    virtual int list_all(std::size_t limit = 100, std::size_t offset = 0) = 0;
    virtual int create(const ExampleUser& u) = 0;           
    virtual int update(const ExampleUser& u) = 0;
    virtual int remove(int id) = 0;
    virtual const char* error() = 0;
    virtual const ExampleUser get_mapped_entry() = 0;
    virtual const std::vector<ExampleUser> get_mapped_entry_vector() = 0;
};


/* 
* STEP 3: MySQL implementation (uses SSQLS + bound params)
*
*/

#if !REPO_FAKE_ONLY
class MySqlExampleUsersRepository : public IExampleUsersRepository
{
public:
        std::reference_wrapper<mysqlpp::Connection> conn_;
        mysqlpp::SimpleResult       simple_result;
        const char*                 error_msg; 
        std::vector<mysqlpp::Row>   rows;
        std::vector<std::string>    names;
        ExampleUser          mapped_entry;
        std::vector<ExampleUser>    mapped_entry_vec;
        mysqlpp::Connection& conn()
        {
            return conn_.get();
        }
    // Immediate syntax validation of input data at initilialization level. 
    // flag things that are definitely wrong; e.g. missing required fields, type mismatch, unparsable strings, any attempts of code injection and the presence (or lack of) of security tokens.
    
    // CTOR overload, allow us to use both a simple conn and scoped one (RAII). both are returning the same type so no further handling is required in the class.  
    explicit MySqlExampleUsersRepository(mysqlpp::Connection& conn)
        : conn_(conn)
    {
        std::cout << "PLACEHOLDER SQL: Server-side validation phase 1\n";
    }

    // Construct from a ScopedConnection (RAII wrapper around Connection)
    explicit MySqlExampleUsersRepository(mysqlpp::ScopedConnection& scoped)
        : conn_(*scoped)
    { // deref to underlying mysqlpp::Connection
        std::cout << "PLACEHOLDER SQL: Server-side validation phase 1\n";
    }

    static ExampleUser make_user(std::string name, std::string email, int id=0)
    {
        ExampleUser ex_user;
        ex_user.id = id;
        ex_user.name = name;
        ex_user.email = email;
        ex_user.created_at = mysqlpp::DateTime(); // fine for tests
        return ex_user;
    }

    int get_by_id(int id) override
    {   
        mysqlpp::Query query = conn().query(
            "SELECT id, name, email, created_at "
            "FROM example_users WHERE id = %0"
        );
        query.parse();
        mysqlpp::StoreQueryResult query_result = query.store(id);
        if (!query_result)
        {
            error_msg = query.error();
            mapped_entry = create_exampleUser();
            return EXIT_FAILURE;
        }
        if (query_result.num_rows() == 0)
        {
            error_msg = "Query not found";
            mapped_entry = create_exampleUser();
            return EXIT_FAILURE;
        }
        mapped_entry = row_to_exampleUser(query_result[0]);
        return EXIT_SUCCESS;
    }

    int list_all(std::size_t limit = 100, std::size_t offset = 0) override
    {
        mysqlpp::Query query = conn().query(
            "SELECT id, name, email, created_at "
            "FROM example_users ORDER BY id DESC LIMIT %0 OFFSET %1"
        );
        query.parse(); 
        mysqlpp::StoreQueryResult query_result = query.store(mysqlpp::sql_int(limit), mysqlpp::sql_int(offset));
        if (query_result)
        {
            mapped_entry_vec.reserve(query_result.num_rows());
            for (const auto& row : query_result)
            {
                mapped_entry_vec.emplace_back(row_to_exampleUser(row));
            }
            return EXIT_SUCCESS;
        }
        else
        {
            error_msg = query.error();
            return EXIT_FAILURE;
        }
    }

    int create(const ExampleUser& ex_user_in) override
    {
        mysqlpp::Query query = conn().query(
            "INSERT INTO example_users (name, email) VALUES (%0q, %1q)"
        );
        query.parse();
        simple_result = query.execute(ex_user_in.name, ex_user_in.email);
        if (simple_result)
        {
            return EXIT_SUCCESS;
        }
        else
        { 
            error_msg = query.error();
            return EXIT_FAILURE;
        }
    }

    int update(const ExampleUser& ex_user) override
    {
        if (ex_user.id == 0)
        { 
            return false;
        }
        mysqlpp::Query query = conn().query(
            "UPDATE example_users SET name=%0q, email=%1q WHERE id=%2"
        );
        query.parse();
        simple_result = query.execute(ex_user.name, ex_user.email, ex_user.id);
        if (simple_result)
        {
            return EXIT_SUCCESS; 
        }
        else
        {
            error_msg = query.error();
            return EXIT_FAILURE; 
        }
    }

    int remove(int id) override
    {
        mysqlpp::Query query = conn().query("DELETE FROM example_users WHERE id=%0");
        query.parse();
        simple_result = query.execute(id);
        if (simple_result)
        {
            return EXIT_SUCCESS;
        }
        else
        { 
            error_msg = query.error();
            return EXIT_FAILURE;
        }
    }

    const char* error()
    {
        return error_msg;
    }

    const std::vector<mysqlpp::Row>& get_rows()
    {
        return rows;
    }

    const std::vector<std::string>& get_names()
    {
        return names;
    }
    
    const ExampleUser get_mapped_entry()
    {
        return mapped_entry;
    }

    void set_mapped_entry(crow::json::rvalue data)
    {
        std::string name = static_cast<std::string>(data["name"]);
        std::string email = static_cast<std::string>(data["email"]);
        mapped_entry.name = name;
        mapped_entry.email = email;
    }
    
    const std::vector<ExampleUser> get_mapped_entry_vector()
    {
        return mapped_entry_vec;
    }

    //   return the number of rows affected by the query.
    my_ulonglong get_affected_rows()
    {
        return simple_result.rows();
    }
    //   return the last value used for an AUTO_INCREMENT field.
    my_ulonglong get_insert_id()
    {
        return static_cast<int>(simple_result.insert_id());
    }
    // return any additional information about the query returned by the server.
    const char* get_info()
    {
        return simple_result.info();
    }
    
    static crow::json::wvalue to_crow_json(const ExampleUser& ex_user)
    {
        crow::json::wvalue obj;
        obj["id"]         = ex_user.id;                        // numeric
        obj["name"]       = std::string(ex_user.name);         // string
        obj["email"]      = std::string(ex_user.email);        // string
        obj["created_at"] = ex_user.created_at.str();          // ISO-like "YYYY-MM-DD HH:MM:SS"
        return obj;
    }

    static crow::json::wvalue to_crow_json(const std::vector<ExampleUser>& ex_users)
    {
        crow::json::wvalue::list arr;
        arr.reserve(ex_users.size());
    for (const auto& ex_user : ex_users)
        {
            arr.push_back(to_crow_json(ex_user));
        }
        return crow::json::wvalue(arr);
    }

 private:
    static ExampleUser create_exampleUser()
    { 
        ExampleUser ex_user;
        ex_user.id = 0;
        ex_user.name = '\0';
        ex_user.email = '\0';
        ex_user.created_at = mysqlpp::DateTime();
        return ex_user;
    }

    static ExampleUser row_to_exampleUser(const mysqlpp::Row& row)
    {
        // Map row → SSQLS in column order above
        ExampleUser ex_user = create_exampleUser();
        ex_user.id         = int(row[0]);
        ex_user.name       = std::string(row[1].c_str());
        ex_user.email      = std::string(row[2].c_str());
        ex_user.created_at = mysqlpp::DateTime(row[3].c_str());
        return ex_user;
    }
};
#endif


/* 
* STEP 4: Use the virtual interface to create a test class
*
*/
#if REPO_ENABLE_FAKE

class FakeExampleUsersRepository : public IExampleUsersRepository
{
public:
    int next_id_ = 1;
    //  A static member exists once total, shared by all objects of that class.
    std::unordered_map<int, ExampleUser> by_id_;
    ExampleUser mapped_entry_;
    std::vector<ExampleUser> mapped_vec_;
    std::string last_error_;

    static ExampleUser make_user(std::string name, std::string email, int id=0)
    {
        ExampleUser ex_user;
        ex_user.id = id;
        ex_user.name = name;
        ex_user.email = email;
        ex_user.created_at = mysqlpp::DateTime(); // fine for tests
        return ex_user;
    }

    int get_by_id(int id) override
    {
        auto it = by_id_.find(id);
        if (it == by_id_.end())
        {
            last_error_ = "not found";
            mapped_entry_ = FakeExampleUsersRepository::make_user("", "", 0);
            return EXIT_FAILURE;
        }
        mapped_entry_ = it->second;
        return EXIT_SUCCESS;
    }

    int list_all(std::size_t limit = 100, std::size_t offset = 0) override
    {
        mapped_vec_.clear();
        mapped_vec_.reserve(std::min(limit, by_id_.size()));
        // stable order by id desc
        std::vector<int> ids;
        ids.reserve(by_id_.size());
        for (auto& kv : by_id_)
        {
            ids.push_back(kv.first);
        }
        std::sort(ids.begin(), ids.end(), std::greater<int>());
        std::size_t i = 0;
        for (int id : ids)
        {
            if (i++ < offset)
            {
                continue;
            }
            if (mapped_vec_.size() >= limit)
            {
                break;
            }
            mapped_vec_.push_back(by_id_[id]);
        }
        return EXIT_SUCCESS;
    }

    int create(const ExampleUser& ex_user) override
    {
        // naive uniqueness check on email to mimic UNIQUE(email)
        for (auto& kv : by_id_)
        {
            if (std::string(kv.second.email) == std::string(ex_user.email))
            {
                last_error_ = "duplicate email";
                return EXIT_FAILURE;
            }
        }
        ExampleUser next_ex_user = ex_user;
        next_ex_user.id = next_id_++;
        by_id_[next_ex_user.id] = next_ex_user;
        mapped_entry_ = next_ex_user;
        return EXIT_SUCCESS;
    }

    int update(const ExampleUser& ex_user) override
    {
        if (ex_user.id == 0 || !by_id_.count(ex_user.id))
        {
            last_error_ = "not found";
            return EXIT_FAILURE;
        }
        // keep unique email invariant
        for (auto& kv : by_id_)
        {
            if (kv.first != ex_user.id && std::string(kv.second.email) == std::string(ex_user.email))
            {
                last_error_ = "duplicate email";
                return EXIT_FAILURE;
            }
        }
        by_id_[ex_user.id] = ex_user;
        mapped_entry_ = ex_user;
        return EXIT_SUCCESS;
    }

    int remove(int id) override
    {
        if (!by_id_.count(id))
        {
            last_error_ = "not found";
            return EXIT_FAILURE;
        }
        by_id_.erase(id);
        return EXIT_SUCCESS;
    }

    const char* error() override
    {
        return last_error_.c_str();
    }
    
    const ExampleUser get_mapped_entry() override
    {
        return mapped_entry_;
    }
    
    const std::vector<ExampleUser> get_mapped_entry_vector() override
    {
        return mapped_vec_;
    }
};
// 
#endif

/* 
* STEP 5: Create some route (crow app + pool_ptr)
*
*/
#if REPO_ENABLE_FAKE
using ExampleUsersRepositoryImpl = FakeExampleUsersRepository;
#else
using ExampleUsersRepositoryImpl = MySqlExampleUsersRepository;
#endif


#if !REPO_FAKE_ONLY

template <typename... Middlewares>
void mysqlExampleUsers_routes(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    // GET /users/:id
    CROW_ROUTE(app, "/exampleusers/<int>").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](int id)
    { 
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        ExampleUsersRepositoryImpl repo(sc);
        
        int result = repo.get_by_id(id);
        if (result == EXIT_SUCCESS)
        {
            ExampleUser ex_user = repo.get_mapped_entry();
            crow::json::wvalue res = repo.to_crow_json(ex_user);

            return crow::response(200, res);
        }
        else
        {
            return crow::response(404, repo.error());
        }
    });

    // GET /users?limit=&offset=
    CROW_ROUTE(app, "/exampleusers")([&pool_ptr](const crow::request& req)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        ExampleUsersRepositoryImpl repo(sc);
        std::size_t limit  = req.url_params.get("limit")  ? std::stoul(req.url_params.get("limit"))  : 100;
        std::size_t offset = req.url_params.get("offset") ? std::stoul(req.url_params.get("offset")) : 0;
        int result = repo.list_all(limit, offset);
        if (result == EXIT_SUCCESS)
        {
            std::vector<ExampleUser> ex_user_vec = repo.get_mapped_entry_vector();
            crow::json::wvalue res = repo.to_crow_json(ex_user_vec);
            return crow::response(200, res);
        }
        else
        {
            return crow::response(204, repo.error());
        }
    });

    // POST /users  { "name": "...", "email": "..." }
    CROW_ROUTE(app, "/exampleusers").methods(crow::HTTPMethod::POST)([&pool_ptr](const crow::request& req)
    { 
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        ExampleUsersRepositoryImpl repo(sc);
        crow::json::rvalue data = crow::json::load(req.body);
        if (!data)
        {
            return crow::response(400, "Invalid JSON - expected list of objects");
        }
        std::string name = static_cast<std::string>(data["name"]);
        std::string email = static_cast<std::string>(data["email"]);
        ExampleUser new_ex_user = repo.make_user(name, email);
        
        bool result = repo.create(new_ex_user);
        if (result == EXIT_FAILURE)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity created");
    });

    // PUT /users/:id  { "name": "...", "email": "..." }
    CROW_ROUTE(app, "/exampleusers/<int>").methods(crow::HTTPMethod::PUT)([&pool_ptr](const crow::request& req, int id)
    { 
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        ExampleUsersRepositoryImpl repo(sc);
        crow::json::rvalue data = crow::json::load(req.body);
        if (!data)
        {
            return crow::response(400, "Invalid JSON - expected list of objects");
        }       
        int result = repo.get_by_id(id);
        ExampleUser ex_user;
        if (result == EXIT_SUCCESS)
        {
            ex_user = repo.get_mapped_entry();
            crow::json::wvalue res = repo.to_crow_json(ex_user);
        }
        else
        {
            return crow::response(404, repo.error());
        }
        repo.set_mapped_entry(data);
        ex_user = repo.get_mapped_entry();
        result = repo.update(ex_user);
        if (result)
        {
            return crow::response(500, repo.error());
        }
        return crow::response(201, "Entity created");
    });

    // DELETE /users/:id
    CROW_ROUTE(app, "/exampleusers/<int>").methods(crow::HTTPMethod::DELETE)([&pool_ptr](int id)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        ExampleUsersRepositoryImpl repo(sc);
        int result = repo.remove(id);
        if (result == EXIT_SUCCESS)
        {
            return crow::response(204);
        }
        else
        {
            return crow::response(404, repo.error());
        }   
    });
}
#endif

#endif

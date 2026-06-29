#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <cstdlib>
#include <stdexcept>
#include <string>


#ifndef CONFIG_STRUCT
#define CONFIG_STRUCT
// struct MySql_config_s
// {
//     std::string host;
//     unsigned int port;
//     std::string user;
//     std::string password;
//     std::string database;
//     unsigned int pool_size;
// };
// typedef struct MySql_config_s MySql_config_t;

enum class DbType
{
      MySQL,
      Redis,
      PostgreSQL
};

struct server_config_s
{
    std::string host;
    std::string bind_addr;
    std::string allowed_origins;
    bool reconnect;
    unsigned short port;
    unsigned int threads;
};
typedef struct server_config_s server_config_t;

struct db_config_s
{
    DbType type;
    std::string host;
    unsigned int port;
    std::string user;
    std::string password;
    std::string database;
    unsigned int pool_size;
};
typedef struct db_config_s db_config_t;

struct app_config_s
{
    server_config_t server_config;
    db_config_t db_config;
};
typedef struct app_config_s app_config_t;

enum class string_code
{
    db_type,
    host,
    bind_addr,
    port,
    user,
    password,
    database,
    pool_size,
    threads,
    allowed_origins,
    unknown
};
#endif
#define _MAX_INPUT_TOKENS_ 15

app_config_t load_config(const char* filename);

#endif

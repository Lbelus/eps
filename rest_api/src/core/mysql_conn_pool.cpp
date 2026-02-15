#include <mysql_conn_pool.hpp>

unsigned int GetThreadCount(unsigned int divBy)
{
     unsigned int nbThread = std::thread::hardware_concurrency();
     unsigned int result = nbThread / divBy;
     return result;
}

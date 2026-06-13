#ifndef INCLUDE_REPOSITORIES_HPP
#define INCLUDE_REPOSITORIES_HPP


// include repositories in no static flags unless you want to use advanced MySqlpp functions. 
// then you should declare the SQLSS as no static inside the reposistories. 

#define MYSQLPP_SSQLS_NO_STATICS
#include <court_doc_repository.hpp>
#undef MYSQLPP_SSQLS_NO_STATICS


#endif


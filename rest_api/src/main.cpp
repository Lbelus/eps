#include <rest_api/rest_api.hpp>
#include <include_repositories.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        rest_api api("./config.yaml");
        return api.start();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>



char* concat_string(int argc, ...)
{
    int len = 0;
    int index = 0;
    char* result = NULL;
	va_list ap;
    va_start(ap, argc);
    while (index < argc)
    {
        len += strlen(va_arg(ap, char*));
        index += 1;
    }
    result = (char*)malloc(sizeof(char)* len + 1);
	result[0] = '\0';
    va_start(ap, argc);
    index = 0;
    while (index < argc)
    {
        strcat(result, va_arg(ap, char*));
        index += 1;
    }
    return result;
}

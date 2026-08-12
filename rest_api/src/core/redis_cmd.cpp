#include <rest_api/redis_client.hpp>


namespace redis_client
{

// Common
    std::string RedisClient::echo(const char *message)
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply *)redisCommand(context, "ECHO %s", message);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STRING)
        {
            return std::string(reply->str, reply->len);
        }
        else 
        {
            throw std::runtime_error("ECHO operation failed.");
        }
    }

    std::string RedisClient::ping()
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply *)redisCommand(context, "PING");
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        return std::string(reply->str, reply->len);
    }

    std::string RedisClient::flushall()
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply *)redisCommand(context, "FLUSHALL");
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        return std::string(reply->str, reply->len);
    }

    std::string RedisClient::info(const char *section)
    {
        redisReply* reply;
        std::string resp;
        if (section)
        {
            reply = (redisReply *)redisCommand(context, "INFO %s", section);
        }
        else
        {
            reply = (redisReply *)redisCommand(context, "INFO");
        }
        
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STRING)
        {
            return std::string(reply->str, reply->len);
        }
        else
        {
            throw std::runtime_error("INFO operation failed.");
        }
    }

// Hashes


    std::string RedisClient::hget(const char* key, const char* field)
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply*)redisCommand(context, "HGET %s %s", key, field);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STRING)
        {
            resp = concatenate("Value of '", field, "': ", reply->str);
            return resp;
        }
        else if (reply->type == REDIS_REPLY_NIL)
        {
            resp = concatenate("Field '", field,"' does not exist.");
            throw std::runtime_error(resp);

        }
        else
        {
            resp = concatenate("Failed to retrieve value for field '", field,"'." );
            throw std::runtime_error(resp);
        }
    }

    std::string RedisClient::hexists(const char* key, const char* field)
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply*)redisCommand(context, "HEXISTS %s %s", key, field);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            if (reply->integer == 1)
            {
                resp = concatenate("Field '", field);
                return resp;
            }
            else
            {
                resp = concatenate("Field '", field, "' does not exist.");
                throw std::runtime_error(resp);
            }
        }
        else
        {
            resp = concatenate("Failed to check existence for field:", field);
            throw std::runtime_error(resp);
        }

    }

    std::string RedisClient::hmset(const char* key, const char** fields, const char** values, size_t fieldCount)
    {
        const char* argv[2 + fieldCount * 2];
        size_t argvlen[2 + fieldCount * 2];
        argv[0] = "HMSET";
        argvlen[0] = 5;
        argv[1] = key;
        argvlen[1] = strlen(key);
        size_t index = 0;
        while (index < fieldCount)
        {
            argv[2 + index * 2] = fields[index];
            argvlen[2 + index * 2] = strlen(fields[index]);
            argv[2 + index * 2 + 1] = values[index];
            argvlen[2 + index * 2 + 1] = strlen(values[index]);
            index += 1;
        }
        redisReply* reply = (redisReply*)redisCommandArgv(context, 2 + fieldCount * 2, argv, argvlen);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STATUS)
        {
            return std::string("Fields set successfully.");
        }
        else
        {
            throw std::runtime_error("Failed to set fields.");
        }
    }

    std::string RedisClient::hdel(const char* key, const char* field)
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply*)redisCommand(context, "HDEL %s %s", key, field);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            resp = concatenate("Number of fields deleted: ",  std::to_string(reply->integer));
            return resp;
        }
        else
        {
            resp = concatenate("Failed to delete field :", field);
            throw std::runtime_error(resp);
        }
    }

    std::string RedisClient::hset(const char* key, const char* field, const char* value)
    {
        redisReply* reply = (redisReply*)redisCommand(context, "HSET %s %s %s", key, field, value);
        std::string resp;
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            if (reply->integer == 1)
            {
                resp = concatenate("Field '", field, "' set for the first time");
                return resp;
            }
            else
            {
                resp = concatenate("Field '", field, "' updated");
                return resp;
            }
        }
        else
        {
            resp = concatenate("Failed to set field: ", field);
            throw std::runtime_error(resp);
        }
    }

    std::vector<std::string> RedisClient::hvals(const char* key)
    {
        std::vector<std::string> vals;
        std::string resp;
        redisReply* reply = (redisReply*)redisCommand(context, "HVALS %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            vals.reserve(reply->elements);
            for (size_t index = 0; index < reply->elements; index++) //iso C++ cannot compare integer and ptr but this works...
            {
                auto* element = reply->element[index];
                if (element->type == REDIS_REPLY_STRING)
                {
                    vals.emplace_back(element->str, element->len);
                }
            }
        }
        else
        {
            resp = concatenate("Failed to retrieve values for hash: ", key);
            throw std::runtime_error(resp);
        }
        return vals;
    }

    std::vector<std::pair<std::string, std::string>> RedisClient::hgetall(const char* key)
    {
        std::vector<std::pair<std::string, std::string>> keys_vals;
        std::string resp;
        redisReply* reply = (redisReply*)redisCommand(context, "HGETALL %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            keys_vals.reserve(reply->elements);
            for (size_t index = 0; index < reply->elements; index += 2) //iso C++ cannot compare integer and ptr but this works...
            {                    
                keys_vals.emplace_back(reply->element[index]->str, reply->element[index + 1]->str);
            }
        }
        else
        {
            resp = concatenate("Failed to retrieve values for hash: ", key);
            throw std::runtime_error(resp);
        }
        return keys_vals;
    }

    std::vector<std::string> RedisClient::hkeys(const char* key)
    {
        std::vector<std::string> keys;
        std::string resp;
        redisReply* reply = (redisReply*)redisCommand(context, "HKEYS %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            keys.reserve(reply->elements);
            for (size_t index = 0; index < reply->elements; index++) //iso C++ cannot compare integer and ptr but this works...
            {
                auto* element = reply->element[index];
                if (element->type == REDIS_REPLY_STRING)
                {
                    keys.emplace_back(element->str, element->len);
                }
            }
        }
        else
        {
            resp = concatenate("Failed to retrieve keys for hash: ", key);
            throw std::runtime_error(resp);
        }
        return keys;
    }

    std::string RedisClient::hlen(const char* key)
    {
        std::string resp;
        redisReply *reply = (redisReply *)redisCommand(context, "HLEN %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            resp = concatenate("Number of fields in hash '", key, "': ", std::to_string(reply->integer));
            return resp;
        }
        else
        {
            resp = concatenate("Failed to get the length of hash: ", key);
            throw std::runtime_error(resp);
        }
    }

// Key_pair

    std::string RedisClient::set(const char* key, const char* value)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "SET %s %s", key, value);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STATUS)
        {
            return std::string(reply->str, reply->len);
        }
        else
        {
            throw std::runtime_error("SET operation failed.");
        }
    }

    std::string RedisClient::get(const char* key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "GET %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STRING)
        {
            return std::string(reply->str, reply->len);
        }
        else
        {
            throw std::runtime_error("GET operation failed or key does not exist.");
        }
    }

    std::vector<std::string> RedisClient::keys(const char *pattern)
    {
        std::vector<std::string> keys; 
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "KEYS %s", pattern);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);

        if (reply->type == REDIS_REPLY_ARRAY)
        {
            keys.reserve(reply->elements); // same as std::unique_ptr<std::string[]> keys(new std::string[reply->elements]); ?
            // printf("Matching keys:\n");
            for (size_t index = 0; index < reply->elements; index++)
            {
                auto* element = reply->element[index];
                if (element->type == REDIS_REPLY_STRING)
                {
                    keys.emplace_back(element->str, element->len);
                }
                // else
                // {
                //     std::cout << "NOT A STRING" << std::endl;
                // } // does element type change in middle of response ?? 
            }
        }
        else
        {
            throw std::runtime_error("No matching keys found.");
        }
        return keys;
    }

    std::string RedisClient::type(const char* key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "TYPE %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STATUS)
        {
            return std::string(reply->str, reply->len);
        }
        else
        {
            std::string errMsg = concatenate("Failed to get type for key ", key);
            throw std::runtime_error(errMsg);
        }
    }

    std::string RedisClient::del(const char* key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "DEL %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            return std::to_string(reply->integer);
        }
        else
        {
            throw std::runtime_error("DEL operation failed.");
        }
    }

    std::string RedisClient::unlink(const char* key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "UNLINK %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            return std::to_string(reply->integer);
        }
        else
        {
            throw std::runtime_error("UNLINK operation failed.\n");
        }
    }

    std::string RedisClient::expire(const char* key, int seconds)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "EXPIRE %s %d", key, seconds);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            if (reply->integer == 1)
            {
                return std::string("EXPIRE operation - timeout set successfully");
            }
            else
            {
                return std::string("EXPIRE operation - key does not exist.");
            }
        }
        else
        {
            throw std::runtime_error("EXPIRE operation failed.");
        }
    }

    std::string RedisClient::rename(const char *old_key, const char *new_key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "RENAME %s %s", old_key, new_key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STATUS)
        {
            return std::string("RENAME operation - key renamed successfully.");
        }
        else
        {
            throw std::runtime_error("RENAME operation failed.");
        }
    }

// List

    std::string RedisClient::lpush(const char* key, const char* value)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "LPUSH %s %s", key, value);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            return std::to_string(reply->integer);
        }
        else
        {
            throw std::runtime_error("LPUSH: Failed to push element to the list.");
        }
    }

    std::string RedisClient::rpush(const char* key, const char* value)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "RPUSH %s %s", key, value);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            return std::to_string(reply->integer);
        }
        else
        {
            throw std::runtime_error("RPUSH: Failed to push element to the list.");
        }
    }

    std::string RedisClient::lpop(const char* key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "LPOP %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STRING)
        {
            return std::string(reply->str);
        }
        else
        {
            throw std::runtime_error("LPOP: List is empty or key does not exist.");
        }
    }

    std::string RedisClient::rpop(const char* key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "RPOP %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STRING)
        {
            return std::string(reply->str);
        }
        else
        {
            throw std::runtime_error("RPOP: List is empty or key does not exist.");
        }
    }

    std::string RedisClient::llen(const char* key)
    {
        redisReply* reply;
        reply = (redisReply*)redisCommand(context, "LLEN %s", key);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            std::string valueToA = std::to_string(reply->integer);
            std::string msg = concatenate(key, valueToA);
            return msg;
        }
        else
        {
            throw std::runtime_error("RPOP: List is empty or key does not exist.");
        }
    }

    std::string RedisClient::lrem(const char* key, int count, const char *element)
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply*)redisCommand(context, "LREM %s %d %s", key, count, element);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            resp = concatenate("Removed ", std::to_string(reply->integer)," occurrences of ", element, key);
            return resp;
        }
        else
        {
            resp = concatenate("Failed to remove elements from list  ", key);
            throw std::runtime_error(resp);
        }
    }

    std::string RedisClient::lindex(const char* key, int index)
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply*)redisCommand(context, "LINDEX %s %d", key, index);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STRING)
        {
            resp = concatenate("Element at index ", std::to_string(reply->integer)," in list :", key, reply->str);
            return resp;
        }
        else
        {
            resp = concatenate("Failed to retrieve element at index ", std::to_string(index)," from list ",  key);
            throw std::runtime_error(resp);
        }
    }

    std::string RedisClient::lset(const char* key, int index, const char* value)
    {
        redisReply* reply;
        std::string resp;
        reply = (redisReply*)redisCommand(context, "LSET %s %d %s", key, index, value);
        if (reply == NULL)
        {
            throw std::runtime_error(context->errstr);
        }
        std::unique_ptr<redisReply, decltype(&freeReplyObject)> autoReply(reply, freeReplyObject);
        if (reply->type == REDIS_REPLY_STATUS)
        {
            return std::string("LSET: Element set successfully.");
        }
        else
        {
            resp = concatenate("LSET: Failed to set element - ", reply->str);
            throw std::runtime_error(resp);
        }
    }
}

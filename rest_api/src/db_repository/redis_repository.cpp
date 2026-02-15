

// // REDIS


// void thread_safe_crow_key(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/key/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.get(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }

// void thread_safe_crow_get(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/get/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.get(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }


// void thread_safe_crow_hmget(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/hmget").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string hash = x["key"].s();
//         std::vector<std::string> fields;
//         
//         for (const auto& item : x["fields"])
//         {
//             fields.push_back(item.s());
//         }

//         std::vector<sw::redis::OptionalString> vals;
//         redis.hmget(hash, fields.begin(), fields.end(), std::back_inserter(vals));
//         crow::json::wvalue result; // https://www.ccoderun.ca/programming/doxygen/Crow/classcrow_1_1json_1_1wvalue.html
//         for (size_t i = 0; i < fields.size(); ++i)
//         {
//             if (vals[i])
//             {
//                 result[fields[i]] = *vals[i];
//             }
//             else
//             {
//                 result[fields[i]] = nullptr;
//             }
//         }
//         return crow::response(200, result);
//     });
// }


// void thread_safe_crow_lpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/lpop/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.lpop(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }

// void thread_safe_crow_rpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/rpop/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.rpop(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }

// void thread_safe_crow_llen(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/llen/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         long long val = redis.llen(key);
//         if (val)
//         {
//             std::string str = std::to_string(val);
//             return crow::response(200, str);
//         }
//         else
//         {
//             return crow::response(404, "List is empty");
//         }
//     });
// }


// void thread_safe_crow_ping(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/ping").methods(crow::HTTPMethod::GET)
//     ([&redis]()
//     {
//         std::string val = redis.ping();
//         if (!val.empty())
//         {
//             return crow::response(200, val);
//         }
//         else
//         {
//             return crow::response(404, "server unresponsive");
//         }
//     });
// }

// void thread_safe_crow_echo(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/echo/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& msg)
//     {
//         std::string val = redis.echo(msg);
//         if (!val.empty())
//         {
//             return crow::response(200, val);
//         }
//         else
//         {
//             return crow::response(404, "server unresponsive");
//         }
//     });
// }

// void thread_safe_crow_flushall(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/flushall").methods(crow::HTTPMethod::GET)
//     ([&redis]()
//     {
//         redis.flushall();
//         return crow::response(200, "data has been flushed");
//     });
// }

// void thread_safe_crow_info(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/info").methods(crow::HTTPMethod::GET)
//     ([&redis]()
//     {
//         std::string  val = redis.info();
//         if (!val.empty())
//         {
//             return crow::response(200, val);
//         }
//         else
//         {
//             return crow::response(404, "Redis could not provide any info");
//         }
//     });
// }


// /// REDIS IS CURRENTLY OUT OF SCOPE

// //redis 

// void  thread_safe_crow_set(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/set").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string key = x["key"].s();
//         std::string value = x["value"].s();

//         redis.set(key, value);

//         return crow::response(200, "Data stored successfully");
//     });
// }

// void  thread_safe_crow_lpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/lpush").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string key = x["key"].s();
//         std::string value = x["value"].s();

//         redis.lpush(key, value);

//         return crow::response(200, "Data stored successfully");
//     });
// }

// void  thread_safe_crow_rpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/rpush").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string key = x["key"].s();
//         std::string value = x["value"].s();

//         redis.rpush(key, value);

//         return crow::response(200, "Data stored successfully");
//     });
// }

// void  thread_safe_crow_hmset(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/hmset").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string hash = x["key"].s();
//         std::unordered_map<std::string, std::string> fields_values;

//         const auto& fields = x["fields_values"];
//         for (auto it = fields.begin(); it != fields.end(); ++it)
//         {
//             fields_values[it->key()] = it->s();
//         }

//         redis.hmset(hash, fields_values.begin(), fields_values.end());

//         return crow::response(200, "Hash data stored successfully");
//     });
// }



// // REDIS


// void simple_crow_key(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/key/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.get(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }

// void simple_crow_get(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/get/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.get(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }


// void simple_crow_hmget(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/hmget").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string hash = x["key"].s();
//         std::vector<std::string> fields;
//         
//         for (const auto& item : x["fields"])
//         {
//             fields.push_back(item.s());
//         }

//         std::vector<sw::redis::OptionalString> vals;
//         redis.hmget(hash, fields.begin(), fields.end(), std::back_inserter(vals));
//         crow::json::wvalue result; // https://www.ccoderun.ca/programming/doxygen/Crow/classcrow_1_1json_1_1wvalue.html
//         for (size_t i = 0; i < fields.size(); ++i)
//         {
//             if (vals[i])
//             {
//                 result[fields[i]] = *vals[i];
//             }
//             else
//             {
//                 result[fields[i]] = nullptr;
//             }
//         }
//         return crow::response(200, result);
//     });
// }


// void simple_crow_lpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/lpop/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.lpop(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }

// void simple_crow_rpop(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/rpop/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         auto val = redis.rpop(key);
//         if (val)
//         {
//             return crow::response(200, *val);
//         }
//         else
//         {
//             return crow::response(404, "Data not found");
//         }
//     });
// }

// void simple_crow_llen(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/llen/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& key)
//     {
//         long long val = redis.llen(key);
//         if (val)
//         {
//             std::string str = std::to_string(val);
//             return crow::response(200, str);
//         }
//         else
//         {
//             return crow::response(404, "List is empty");
//         }
//     });
// }


// void simple_crow_ping(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/ping").methods(crow::HTTPMethod::GET)
//     ([&redis]()
//     {
//         std::string val = redis.ping();
//         if (!val.empty())
//         {
//             return crow::response(200, val);
//         }
//         else
//         {
//             return crow::response(404, "server unresponsive");
//         }
//     });
// }

// void simple_crow_echo(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/echo/<string>").methods(crow::HTTPMethod::GET)
//     ([&redis](const std::string& msg)
//     {
//         std::string val = redis.echo(msg);
//         if (!val.empty())
//         {
//             return crow::response(200, val);
//         }
//         else
//         {
//             return crow::response(404, "server unresponsive");
//         }
//     });
// }

// void simple_crow_flushall(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/flushall").methods(crow::HTTPMethod::GET)
//     ([&redis]()
//     {
//         redis.flushall();
//         return crow::response(200, "data has been flushed");
//     });
// }

// void simple_crow_info(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/info").methods(crow::HTTPMethod::GET)
//     ([&redis]()
//     {
//         std::string  val = redis.info();
//         if (!val.empty())
//         {
//             return crow::response(200, val);
//         }
//         else
//         {
//             return crow::response(404, "Redis could not provide any info");
//         }
//     });
// }


// /// REDIS IS CURRENTLY OUT OF SCOPE

// //redis 

// void  simple_crow_set(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/set").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string key = x["key"].s();
//         std::string value = x["value"].s();

//         redis.set(key, value);

//         return crow::response(200, "Data stored successfully");
//     });
// }

// void  simple_crow_lpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/lpush").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string key = x["key"].s();
//         std::string value = x["value"].s();

//         redis.lpush(key, value);

//         return crow::response(200, "Data stored successfully");
//     });
// }

// void  simple_crow_rpush(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/rpush").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string key = x["key"].s();
//         std::string value = x["value"].s();

//         redis.rpush(key, value);

//         return crow::response(200, "Data stored successfully");
//     });
// }

// void  simple_crow_hmset(crow::Crow<Middlewares...>& app, sw::redis::Redis& redis)
// {
//     CROW_ROUTE(app, "/hmset").methods(crow::HTTPMethod::POST)
//     ([&redis](const crow::request& req)
//     {
//         auto x = crow::json::load(req.body);
//         if (!x)
//         {
//             return crow::response(400, "Invalid JSON");
//         }

//         std::string hash = x["key"].s();
//         std::unordered_map<std::string, std::string> fields_values;

//         const auto& fields = x["fields_values"];
//         for (auto it = fields.begin(); it != fields.end(); ++it)
//         {
//             fields_values[it->key()] = it->s();
//         }

//         redis.hmset(hash, fields_values.begin(), fields_values.end());

//         return crow::response(200, "Hash data stored successfully");
//     });
// }




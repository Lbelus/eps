# rest_api
a basic cpp rest api


## PROJECT

### **1. Multi-Threaded Connection Management**

1.1 **MySQL Connection Pool**

* 1.1.1 Design thread-safe connection pool class -> ok
* 1.1.2 Implement connection acquisition and release logic -> ok
* 1.1.3 Implement idle connection timeout and auto-reconnect -> ok
* 1.1.4 Test under concurrent request load

1.2 **Redis Connection Pool**

* 1.2.1 Implement thread-safe pool for Redis connections
* 1.2.2 Integrate with async Redis client (if needed)
* 1.2.3 Benchmark performance in multi-threaded mode

---

### **2. Input Validation Layer**

2.1 **Request Parsing & Sanitization**

* 2.1.1 Validate JSON body schema
* 2.1.2 Validate query parameters and types
* 2.1.3 Escape unsafe inputs for SQL & Redis commands string request

2.2 **Validation Framework Integration**

* 2.2.1 Implement reusable validation utilities -> ok
* 2.2.2 Add error handling and return standardized HTTP error codes -> ok

---

### **3. Secure Connection and Validation**

3.1 **HTTPS Configuration**

* 3.1.1 Enable TLS/SSL in Crow server -> ok
* 3.1.2 Load and manage SSL certificates

3.2 **Connection Validation**

* 3.2.1 Enforce HTTPS-only connections
* 3.2.2 Validate certificate handshake on incoming connections

3.3 **Optional Security Hardening**

* 3.3.1 Implement rate limiting
* 3.3.2 Enable CORS and security headers 

---

### **4. Testing & Quality Assurance**

4.1 **Unit Tests (Google Test)**

* 4.1.1 Test connection pool logic under concurrent load
* 4.1.2 Test input validation logic with edge cases -> test framework in place the rest is CI/CD
* 4.1.3 Test HTTPS connection establishment and failures

4.2 **Integration Tests**

* 4.2.1 Test API endpoints with mocked MySQL
* 4.2.2 Test API endpoints with mocked Redis
* 4.2.3 Test multi-threaded performance and resource usage

---

### **5. Documentation & Developer Support**

5.1 **Developer Documentation**

* 5.1.1 Document API endpoints with examples
* 5.1.2 Document connection pool usage and limits
* 5.1.3 Document validation and security considerations

5.2 **Setup & Deployment Guide**

* 5.2.1 Document SSL setup and certificate management
* 5.2.2 Provide Docker or CMake build instructions -> ok

## Installation for development purposes

1. Install docker : https://docs.docker.com/engine/install/ubuntu/ 

2. From the projet directory ``source`` the bash helper script and build the environment
```bash
source bash_scripts/helper_script.sh   
rest_api_build_dev
rest_api_run_dev
```

3. You can stop the containers with ``rest_api_stop_dev`` and start the container with ``rest_api_start_dev``.

4. (Optional) You can start a web container MySqlWorkbench interface:
You can store the certificate in the ``./ssl`` directory
```bash
bash ./bash_scripts/my_sql_workbench_non_hardened.sh
```
Once the script has run, ddl the ``.p12`` on your local machine using ``scp`` and install the ``.p12`` certificate into your web browser.
This script will start a nginx reverse proxy to handle a secure connection to server and link to the MySqlWorkbench web container. 

You should be able to log via https:://<server_ip>:<port>

/!\ The script is non hardened and for development purposes ONLY /!\

## Usage

### How to compile and run the tests

/!\ don't forget to ``source bash_scripts/helper_script.sh`` each time you start the server and the container. 

From the rest api container ``cont_llvm_mysql_crow`` you can build the software with using the following helper function: 

- To build, compile the soft and launch it using light compilation flags such as  ``-Wall -Wextra -Werror -fsanitize=address``
    -> From the project directory run 
```bash
re
```
- To build, compile the soft and launch it using hard compilation flags such as ``-g3 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-snig-conversion -fsanitize=address``
    -> From the project directory run:
```bash
re_full
```
- To build, compile the soft with the tests objects and performs the tests using google test.
    -> From the project directory run:
```bash
go_tests
```

- To perfom external test against the http server.
    -> From the outside the rest api run:
```bash
bash ./tests/<your_external_script_test.sh>
``` 
-> You can try it out with ``./tests/external_ExampleUser_route_tests.sh`` 

### How to perform some basic manual tests with the helper_script 

1. Recover the container ip with: 
```bash
rest_api_get_container_ip cont_llvm_mysql_crow
```
2. (Optional) if the table has not been initialized, run:
```bash
rest_api_init_db
```
You can also drop the db with:
```bash
rest_api_drop_db
```

3. You can run any of the following command from the ``bash_scripts/helper_script.sh`` file, or add some more, to test out the rest api interaction, it will rely on the string repository routes:
```md
<call>           ::= "rest_api_test_read_all(" <ip_port> ")"
                   | "rest_api_test_read_by_id(" <ip_port> "," <id> ")"
                   | "rest_api_test_create_entity(" <ip_port> "," <entry> ")"
                   | "rest_api_test_update_entity(" <ip_port> "," <id> "," <entry> ")"
                   | "rest_api_test_delete_entity(" <ip_port> "," <id> ")"
                   | "rest_api_test_join_entity(" <ip_port> ")"
                   | "rest_api_test_order_entity(" <ip_port> ")"
                   | "rest_api_test_read(" <tarball> ")"

<ip_port>        ::= <ip> ":" <port>
```

## Development guidelines

### general guidelines and information.

- This project follows a REPOSITORY DESIGN PATTERN;
- It relies on MySql++, redis-plus-plus, crow and google tests, you can find more information on thoses library in the documentation segment.
- A string repository as been developped as a clutch to help testing your table and result, it is not to be used in production. 
- An Example class has been developped to provide an example of what is expected to be done when you create a new class. 
    -> you can find it at ``./src/db_repository/example_repository.hpp`` 
- Unless you have some very specific case where you need to use a ``.tpp`` file this project will header only.
- One class/transaction per header.

### Development steps. 

1. Define the model with SSQLS:
- Create a SSQLS table with ``sql_create``;
- Add the MySql equivalent in the comments.

2. Create the Repository virtual interface (exampleUsers):
- It contains the functions that will be overriden inside your prod and your test repositories.

3. MySQL (or later on redis) implementation (uses SSQLS + bound params):
- This is where you implement your business logic, handle the data, the result;
- The class will peform mostly CRUD interaction with the DB and mapping.

4. Use the virtual interface to create a test class:
- Perform local test without a DB;
- Perform non regressions tests.

5. Create some route (crow app + pool_ptr):
- Call the connection object, the repository, perform the transaction and sent the response along with an http status code.

6. Add the routes in the ``funct_ptr`` map present (for now) in the ``main.cpp``.

7. Write the tests with ``google tests`` in a ``.cc`` file in the ``./tests/`` dir:
- Tests your function
- Add non regressions test each time your perform a correction. 

8. Write some external tests with bash to test out the http server and db interraction in ``./tests/``.

9. Describe the usage in the ``REST API USAGE`` segment.

You will find some examples, comments and guidelines in the following file: 
- ``./src/db_repository/example_repository.hpp``
- ``./tests/test_example_repo.cc``
- ``./tests/external_ExampleUser_route_tests.sh``


You can also look at the string repository, it's implementation differs slightly as it relies on .cpp and .tpp file.
The project relies on virtual interfaces and some very light template programming, which means that header only is on standard.


/!\ The string repository is missing some important validation steps and is not be used in production /!\

## MySQL vs MySqlpp types


### tldr

* `INT` → `mysqlpp::sql_int`
* `INT UNSIGNED` → `mysqlpp::sql_int_unsigned`
* `BIGINT` → `mysqlpp::sql_bigint`
* `VARCHAR(n)` → `mysqlpp::sql_varchar`
* `TEXT` → `mysqlpp::sql_text`
* `DATETIME` / `TIMESTAMP` → `mysqlpp::sql_datetime` or `sql_timestamp`
* `DATE` → `mysqlpp::sql_date`
* `TINYINT(1)` used as bool → `mysqlpp::sql_bool`
* Any of the above `NULL`able → append `_null` (e.g. `sql_int_null`, `sql_varchar_null`)

### Integer types

| MySQL type           | mysql++ SSQLS type                | Underlying C++ type (typical) |
| -------------------- | --------------------------------- | ----------------------------- |
| `TINYINT`            | `mysqlpp::sql_tinyint`            | 8-bit signed (int8_t)         |
| `TINYINT UNSIGNED`   | `mysqlpp::sql_tinyint_unsigned`   | 8-bit unsigned (uint8_t)      |
| `SMALLINT`           | `mysqlpp::sql_smallint`           | 16-bit signed (int16_t)       |
| `SMALLINT UNSIGNED`  | `mysqlpp::sql_smallint_unsigned`  | 16-bit unsigned (uint16_t)    |
| `MEDIUMINT`          | `mysqlpp::sql_mediumint`          | 24/32-bit signed (int32_t)    |
| `MEDIUMINT UNSIGNED` | `mysqlpp::sql_mediumint_unsigned` | 24/32-bit unsigned (uint32_t) |
| `INT` / `INTEGER`    | `mysqlpp::sql_int`                | 32-bit signed (int32_t)       |
| `INT UNSIGNED`       | `mysqlpp::sql_int_unsigned`       | 32-bit unsigned (uint32_t)    |
| `BIGINT`             | `mysqlpp::sql_bigint`             | 64-bit signed (int64_t)       |
| `BIGINT UNSIGNED`    | `mysqlpp::sql_bigint_unsigned`    | 64-bit unsigned (uint64_t)    |

Extra integer aliases that exist in mysql++:

| Alias type      | MySQL “equivalent” | Notes                                         |
| --------------- | ------------------ | --------------------------------------------- |
| `sql_int1`      | `TINYINT`          | alias of `sql_tinyint` ([tangentsoft.com][1]) |
| `sql_int2`      | `SMALLINT`         | alias of `sql_smallint`                       |
| `sql_int3`      | `MEDIUMINT`        | alias of `sql_mediumint`                      |
| `sql_int4`      | `INT`              | alias of `sql_int`                            |
| `sql_int8`      | `BIGINT`           | alias of `sql_bigint`                         |
| `sql_middleint` | `MEDIUMINT`        | another alias of `sql_mediumint`              |

And booleans:

| MySQL type                                                                | mysql++ SSQLS type                           | Notes                             |
| ------------------------------------------------------------------------- | -------------------------------------------- | --------------------------------- |
| `BOOLEAN` / `BOOL` (stored as `TINYINT(1)` in MySQL) ([dev.mysql.com][2]) | `mysqlpp::sql_bool` / `mysqlpp::sql_boolean` | Both are aliases of `sql_tinyint` |

---

### Floating / decimal types

| MySQL type                    | mysql++ SSQLS type     | Underlying C++ type    |
| ----------------------------- | ---------------------- | ---------------------- |
| `FLOAT`                       | `mysqlpp::sql_float`   | `float`                |
| `DOUBLE` / `DOUBLE PRECISION` | `mysqlpp::sql_double`  | `double`               |
| `DECIMAL(M,D)` / `NUMERIC`    | `mysqlpp::sql_decimal` | `double` (approx)      |
| (alternate names)             | `mysqlpp::sql_numeric` | alias of `sql_decimal` |
| (alternate)                   | `mysqlpp::sql_fixed`   | alias of `sql_decimal` |
| (aliases)                     | `mysqlpp::sql_float4`  | alias of `sql_float`   |
|                               | `mysqlpp::sql_float8`  | alias of `sql_double`  |

> Note: `sql_decimal` is mapped to `double` in mysql++, so it’s not *exact* decimal arithmetic, just like the docs say. ([tangentsoft.com][1])

---

### Character & text types

All character/text types ultimately map to `std::string` (or `mysqlpp::String` for some blob variants). ([tangentsoft.com][1])

| MySQL type   | mysql++ SSQLS type        |
| ------------ | ------------------------- |
| `CHAR(n)`    | `mysqlpp::sql_char`       |
| `VARCHAR(n)` | `mysqlpp::sql_varchar`    |
| `TINYTEXT`   | `mysqlpp::sql_tinytext`   |
| `TEXT`       | `mysqlpp::sql_text`       |
| `MEDIUMTEXT` | `mysqlpp::sql_mediumtext` |
| `LONGTEXT`   | `mysqlpp::sql_longtext`   |

Aliases:

| mysql++ alias type      | Approx MySQL equivalent |
| ----------------------- | ----------------------- |
| `sql_long_varchar`      | `MEDIUMTEXT`            |
| `sql_long`              | `MEDIUMTEXT`            |
| `sql_character_varying` | `VARCHAR`               |

---

### Binary / BLOB types

These are defined once `mystring.h` is included and use `mysqlpp::String` internally. ([tangentsoft.com][1])

| MySQL type       | mysql++ SSQLS type                                        |
| ---------------- | --------------------------------------------------------- |
| `BLOB`           | `mysqlpp::sql_blob`                                       |
| `TINYBLOB`       | `mysqlpp::sql_tinyblob`                                   |
| `MEDIUMBLOB`     | `mysqlpp::sql_mediumblob`                                 |
| `LONGBLOB`       | `mysqlpp::sql_longblob`                                   |
| (like VARBINARY) | `mysqlpp::sql_long_varbinary` (alias of `sql_mediumblob`) |

---

### Date & time types

These map to mysql++’s own `Date`, `Time`, and `DateTime` classes. ([tangentsoft.com][1])

| MySQL type  | mysql++ SSQLS type       | Underlying C++ class |
| ----------- | ------------------------ | -------------------- |
| `DATE`      | `mysqlpp::sql_date`      | `mysqlpp::Date`      |
| `TIME`      | `mysqlpp::sql_time`      | `mysqlpp::Time`      |
| `DATETIME`  | `mysqlpp::sql_datetime`  | `mysqlpp::DateTime`  |
| `TIMESTAMP` | `mysqlpp::sql_timestamp` | `mysqlpp::DateTime`  |

(`YEAR` doesn’t have a dedicated `sql_year`; you usually treat it as an `INT` or `SMALLINT`.)

---

### ENUM / SET

| MySQL type  | mysql++ SSQLS type  | Underlying C++ type |
| ----------- | ------------------- | ------------------- |
| `ENUM(...)` | `mysqlpp::sql_enum` | `std::string`       |
| `SET(...)`  | `mysqlpp::sql_set`  | `mysqlpp::Set<>`    |

`sql_set` is only defined when `myset.h` is included. ([tangentsoft.com][1])

---

### Nullable variants

For every `sql_*` type, mysql++ also gives you a `*_null` type if you include `null.h`. These correspond to `NULL`able columns and wrap the value in `mysqlpp::Null<T>`. ([tangentsoft.com][1])

Examples:

* `mysqlpp::sql_int_null` → nullable `INT`
* `mysqlpp::sql_varchar_null` → nullable `VARCHAR`
* `mysqlpp::sql_datetime_null` → nullable `DATETIME`
* `mysqlpp::sql_blob_null` → nullable `BLOB`
* `mysqlpp::sql_set_null` → nullable `SET`


---

## Documentation


### MySql++:

- https://tangentsoft.com/mysqlpp/doc/html/userman/
- https://tangentsoft.com/mysqlpp/wiki?name=MySQL%2B%2B&p&nsm

### MySql++ - connection and multi-threading:
- https://tangentsoft.com/mysqlpp/doc/html/userman/threads.html
- https://github.com/rpetrich/mysqlpp/blob/master/examples/cpool.cpp

### https/server crow documentation.

- https://crowcpp.org/master/guides/routes/

### redis-plus-plus documentation

- https://github.com/sewenew/redis-plus-plus/blob/master/src/sw/redis%2B%2B/redis.h

### googleTest documentation
- https://google.github.io/googletest/
- https://github.com/google/googletest/tree/main/googletest/samples



## REST API USAGE

### /exampleusers repository

| HTTP   | Route                | Body / Query                                       | Example                                                                                                                                                           |
| ------ | -------------------- | -------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GET    | `/exampleusers/<id>` | path: `id:int`                                     | `curl -X GET http://localhost:18080/exampleusers/1`                                                                                                               |
| GET    | `/exampleusers`      | query: `limit` (default 100), `offset` (default 0) | `curl -X GET "http://localhost:18080/exampleusers?limit=10&offset=0"`                                                                                             |
| POST   | `/exampleusers`      | JSON: `{"name":"...","email":"..."}`               | `curl -X POST http://localhost:18080/exampleusers \`<br>`-H "Content-Type: application/json" \`<br>`-d '{"name":"Jean","email":"jean.jean@email.com"}'`           |
| PUT    | `/exampleusers/<id>` | JSON: `{"name":"...","email":"..."}`               | `curl -X PUT http://localhost:18080/exampleusers/1 \`<br>`-H "Content-Type: application/json" \`<br>`-d '{"name":"Antoine","email":"antoine.antoine@email.com"}'` |
| DELETE | `/exampleusers/<id>` | path: `id:int`                                     | `curl -X DELETE http://localhost:18080/exampleusers/1`                                                                                                            |

### string repository

| HTTP   | Route                             | Arguments                                                 | Example                                                                                                                                              |                                                           |
| ------ | --------------------------------- | --------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------- |
| GET    | `/read/<table>`                   | path: `table:string`                                      | `curl -X GET http://localhost:18080/read/users`                                                                                                      |                                                           |
| GET    | `/read/<table>/<id>`              | path: `table:string`, `id:int`                            | `curl -X GET http://localhost:18080/read/users/1`                                                                                                    |                                                           |
| GET    | `/join/<tableA>/<tableB>`         | path: `tableA:string`, `tableB:string`                    | `curl -X GET http://localhost:18080/join/users/orders`                                                                                               |                                                           |
| GET    | `/order/<table>/<column>/<order>` | path: `table:string`, `column:string`, `order:string (asc | desc)`                                                                                                                                               | `curl -X GET http://localhost:18080/order/users/name/asc` |
| POST   | `/insert/<table>`                 | path: `table:string` + JSON array of objects              | `curl -X POST http://localhost:18080/insert/users \`<br>`-H "Content-Type: application/json" \`<br>`-d '[{"name":"Jean","email":"jean.jean@email.com"}]'` |                                                           |
| PUT    | `/update/<table>/<id>`            | path: `table:string`, `id:int` + JSON object of fields    | `curl -X PUT http://localhost:18080/update/users/1 \`<br>`-H "Content-Type: application/json" \`<br>`-d '{"name":"Antoine"}'`                           |                                                           |
| DELETE | `/delete/<table>/<id>`            | path: `table:string`, `id:int`                            | `curl -X DELETE http://localhost:18080/delete/users/1`                                                                                               |                                                           |


### REDIS USAGE (redis is currently out of scope)

| Category | Redis cmd    | Arguments       | Examples           |
| -------- | -------- | --------------- | --------------- |
| POST     | set      | set             | curl http://localhost:80/set \ <br> -H "Content-Type: application/json" \ <br> -d '{"key":"mykey", "value":"myvalue"}'|
| POST     | lpush    | lpush           | curl http://localhost:80/lpush \ <br> -H "Content-Type: application/json" \ <br> -d '{"key":"mykey", "value":"myvalue"}'|
| POST     | rpush    | rpush           | curl http://localhost:80/rpush \ <br> -H "Content-Type: application/json" \ <br> -d '{"key":"mykey", "value":"myvalue"}'|
| POST     | hmset    | hmset           | curl http://localhost:80/rpush \ <br> -H "Content-Type: application/json" \ <br> -d '{"key":"myhash", "fields_values":{"field1":"value1", "field2":"value2"}}'|
| POST     | hmget    | hmget           | curl http://localhost:80/hmget \ <br> -H "Content-Type: application/json" \ <br> -d '{"key":"myhash", "fields":["field1", "field2"]}'|
| GET      | key      | key/<_string_>  | curl http://localhost:80/key/key_value|
| GET      | get      | key/<_string_>  | curl http://localhost:80/get/key_value|
| GET      | lpop     | lpop/<_string_> | curl http://localhost:80/lpop/key_value|
| GET      | rpop     | rpop/<_string_> | curl http://localhost:80/rpop/key_value|
| GET      | llen     | llen/<_string_> | curl http://localhost:80/llen/key_value|
| GET      | ping     | ping            | curl http://localhost:80/ping|
| GET      | echo     | echo/<_string_> | curl http://localhost:80/echo/msg|
| GET      | flushall | flushall        | curl http://localhost:80/flushall|
| GET      | info     | info            | curl http://localhost:80/info|

Complementary information:
``to test with self signed cert use -k flag``
post request :

```sh
curl http://localhost:80/post \
    -H "Content-Type: application/json" \
    -d '{"key":"mykey", "value":"myvalue"}'
```

get request:
```sh
curl http://localhost:80/get/mykey
```



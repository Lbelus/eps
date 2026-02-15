#!/bin/bash

rest_api_build_dev()
{
    sudo docker network create sqlRest
    sudo docker build -t img_llvm_mysql_crow .
    sudo docker pull mysql:latest
}

rest_api_run_dev()
{
    sudo docker run --name mysqlserver --network sqlRest -e MYSQL_ROOT_PASSWORD=your_root_password -e MYSQL_DATABASE=test_rest_DB -e MYSQL_USER=dev_admin -e MYSQL_PASSWORD=dev_admin -v mysql_data_test_rest:/var/lib/mysql -p 3306:3306 -d mysql:8.0
    sudo docker run -it --network sqlRest -v .:/workspace --name cont_llvm_mysql_crow img_llvm_mysql_crow /bin/bash
}

rest_api_start_dev()
{
    sudo docker start mysqlserver
    sudo docker start -ai cont_llvm_mysql_crow
}

rest_api_stop_dev()
{
    sudo docker stop mysqlserver
    sudo docker stop cont_llvm_mysql_crow
}

rest_api_rm_dev()
{
    sudo docker rm mysqlserver
    sudo docker rm cont_llvm_mysql_crow
}

rest_api_test_read_all()
{
    ip_port=$1
    curl -X GET http://$ip_port/read/users
}

rest_api_test_read_by_id()
{
    ip_port=$1
    id=$2
    curl -X GET http://$ip_port/read/users/$id
}

rest_api_test_insert_entity()
{
    ip_port=$1
    entry=$2
    curl -X POST http://$ip_port/insert/users \
        -H "Content-Type: application/json" \
        -d "[{\"name\": \"$entry\"}]"
}

rest_api_test_update_entity()
{
    ip_port=$1
    id=$2
    entry=$3
    curl -X PUT http://$ip_port/update/users/$id \
        -H "Content-Type: application/json" \
        -d "{\"name\": \"$entry\"}"
}

rest_api_test_delete_entity()
{
    ip_port=$1
    id=$2
    curl -X DELETE http://$ip_port/delete/users/$id
}

rest_api_test_join_entity()
{
    ip_port=$1
    curl http://$ip_port/join/users/orders
}

rest_api_test_order_entity()
{
    ip_port=$1
    curl http://$ip_port/order/users/name/ASC
}

rest_api_get_container_ip()
{
    arg=$1
    sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $arg
}

rest_api_init_db()
{
    sudo docker exec -i mysqlserver mysql -u dev_admin -pdev_admin test_rest_DB <<EOF
    CREATE TABLE IF NOT EXISTS users (
        id INT AUTO_INCREMENT PRIMARY KEY,
        name VARCHAR(100) NOT NULL
    );

    INSERT INTO users (name) VALUES ('Alice'), ('Bob');
    
    CREATE TABLE IF NOT EXISTS orders (
        id INT AUTO_INCREMENT PRIMARY KEY,
        designation VARCHAR(100) NOT NULL,
        users_id INT NOT NULL,
        FOREIGN KEY (users_id) REFERENCES users(id)
    );

   INSERT INTO orders (designation, users_id) VALUES 
    ('Potato', 1),
    ('Pasta', 2);
EOF
}

rest_api_drop_db()
{
    sudo docker exec -i mysqlserver mysql -u dev_admin -pdev_admin test_rest_DB <<EOF
        DROP TABLE IF EXISTS orders;
        DROP TABLE IF EXISTS users;
EOF
}


rest_api_test_read_all()
{
    ip_port=$1
    curl -X GET http://$ip_port/read/users
}

rest_api_test_read_by_id()
{
    ip_port=$1
    id=$2
    curl -X GET http://$ip_port/read/users/$id
}

rest_api_test_create_entity()
{
    ip_port=$1
    entry=$2
    curl -X POST http://$ip_port/create/users \
        -H "Content-Type: application/json" \
        -d "[{\"name\": \"$entry\"}]"
}

rest_api_test_update_entity()
{
    ip_port=$1
    id=$2
    entry=$3
    curl -X PUT http://$ip_port/update/users/$id \
        -H "Content-Type: application/json" \
        -d "{\"name\": \"$entry\"}"
}

rest_api_test_delete_entity()
{
    ip_port=$1
    id=$2
    curl -X DELETE http://$ip_port/delete/users/$id
}

rest_api_test_join_entity()
{
    ip_port=$1
    curl http://$ip_port/join/users/orders
}

rest_api_test_order_entity()
{
    ip_port=$1
    curl http://$ip_port/order/users/name/asc
}

rest_api_test_read()
{
    tarball=$1
    tar -xf tarball
}

re()
{
    clear
    cd build/
    rm -r *
    cmake .. -DENABLE_BASIC_FLAGS=ON 
    make
    cd ..
    ./build/rest_api
}

re_full()
{
    clear
    cd build/
    rm -r *
    cmake .. -DENABLE_FULL_FLAGS=ON
    make
    cd ..
    ./build/rest_api
}

go_tests()
{
    clear
    cd build/
    rm -r *
    cmake .. -DENABLE_GTEST=ON
    make repo_tests
    ctest --test-dir . --output-on-failure
    cd ..
}

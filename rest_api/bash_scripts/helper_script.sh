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
    sudo docker run -it --network sqlRest -p 3004:3004 -v .:/workspace --name cont_llvm_mysql_crow img_llvm_mysql_crow /bin/bash
}

rest_api_backup_db()
{
    sudo docker exec mysqlserver \
      sh -c 'mysqldump -u root -p"your_root_password" --single-transaction --routines --triggers test_rest_DB' \
      > db_backup$(date +%Y%m%d_%H%M%S).sql
}

rest_api_restore_db()
{
    db_name=$1
    backup_path=$2
    mysql -u root -p $db_name < $backup_path
}

front_end_run_dev()
{
    sudo docker run -it --network sqlRest -p 8084:3000 -v ./:/workspace --name cont_eps_front img_eps_front
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

rest_api_init_documents_db()
{
    sudo docker exec -i mysqlserver mysql -u dev_admin -pdev_admin test_rest_DB <<'EOF'
    CREATE TABLE IF NOT EXISTS documents (
        document_id INT AUTO_INCREMENT PRIMARY KEY,
        filename    VARCHAR(512) NOT NULL UNIQUE,
        source      VARCHAR(255) NOT NULL,
        page_count  INT NOT NULL,
        full_text   LONGTEXT NOT NULL,
        created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FULLTEXT KEY ft_documents_full_text (full_text),
        INDEX idx_documents_source (source)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

    CREATE TABLE IF NOT EXISTS pages (
        page_id      INT AUTO_INCREMENT PRIMARY KEY,
        document_id  INT NOT NULL,
        page_number  INT NOT NULL,
        page_text    LONGTEXT NOT NULL,
        INDEX idx_pages_document (document_id),
        UNIQUE KEY uq_pages_document_page (document_id, page_number),
        CONSTRAINT fk_pages_document
            FOREIGN KEY (document_id)
            REFERENCES documents(document_id)
            ON DELETE CASCADE
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
EOF
}

rest_api_test_migration()
{
    sudo docker exec -i mysqlserver mysql -u dev_admin -pdev_admin test_rest_DB -N -e "
        SELECT 'documents', COUNT(*) FROM documents;
        SELECT 'pages', COUNT(*) FROM pages;
    "
}


rest_api_drop_db()
{    
    sudo docker exec -i mysqlserver mysql -u dev_admin -pdev_admin test_rest_DB <<'EOF'
        DROP TABLE IF EXISTS pages;
        DROP TABLE IF EXISTS documents;
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

# --------------------
rest_api_test_documents_read_all()
{
    ip_port=$1
    curl -X GET "http://$ip_port/courtdocuments"
}

rest_api_test_documents_read_all_paginated()
{
    ip_port=$1
    limit=$2
    offset=$3
    curl -X GET "http://$ip_port/courtdocuments?limit=$limit&offset=$offset"
}

rest_api_test_documents_read_by_id()
{
    ip_port=$1
    id=$2
    curl -X GET "http://$ip_port/courtdocuments/$id"
}

rest_api_test_document_pages()
{
    ip_port=$1
    document_id=$2
    curl -X GET "http://$ip_port/courtdocuments/$document_id/pages"
}

rest_api_test_documents_search()
{
    ip_port=$1
    query=$2
    curl -G -X GET "http://$ip_port/courtdocuments/search" \
        --data-urlencode "q=$query"
}

rest_api_test_documents_search_paginated()
{
    ip_port=$1
    query=$2
    limit=$3
    offset=$4
    curl -G -X GET "http://$ip_port/courtdocuments/search" \
        --data-urlencode "q=$query" \
        --data-urlencode "limit=$limit" \
        --data-urlencode "offset=$offset"
}

rest_api_test_documents_smoke()
{
    ip_port=$1
    doc_id=$2
    query=$3

    echo "==== READ ALL ===="
    curl -s -X GET "http://$ip_port/courtdocuments" | jq

    echo
    echo "==== READ BY ID ===="
    curl -s -X GET "http://$ip_port/courtdocuments/$doc_id" | jq

    echo
    echo "==== DOCUMENT PAGES ===="
    curl -s -X GET "http://$ip_port/courtdocuments/$doc_id/pages" | jq

    echo
    echo "==== SEARCH ===="
    curl -s -G -X GET "http://$ip_port/courtdocuments/search" \
        --data-urlencode "q=$query" | jq
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

re_beta()
{
    clear
    cd build/
    rm -r *
    cmake .. -DENABLE_BETA_FLAGS=ON
    make
    cd ..
    ./build/rest_api
}


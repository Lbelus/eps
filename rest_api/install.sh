#!/bin/bash

sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    unzip \
    libmysqlclient21 \
    libhiredis-dev \
    mysql-client \
    nginx \
    apt-get autoremove -y && \
    mysql-server \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

sudo snap install --classic certbot \
    ln -s /snap/bin/certbot /usr/bin/certbot \
    snap set certbot trust-plugin-with-root=ok \
    snap install certbot-dns-cloudflare

wget -O redis-plus-plus.zip https://github.com/sewenew/redis-plus-plus/archive/refs/heads/master.zip && \
    unzip redis-plus-plus.zip && \
    cd redis-plus-plus-master && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install && \
    cd ../../ && \
    rm -rf redis-plus-plus.zip redis-plus-plus-master

systemctl enable --now mysql

#source bash_scripts/helper_script.sh
# rest_api_restore_db

#mysql -u root -p -e "CREATE DATABASE IF NOT EXISTS test_rest_DB;"
#mysql -u root -p test_rest_DB -e "SHOW TABLES;"
#mysql -u root -p -e "SHOW VARIABLES LIKE 'port';"

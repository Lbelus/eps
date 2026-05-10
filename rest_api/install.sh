sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    unzip \
    libmysqlclient21 \
    libhiredis-dev \
    mysql-client \ 
    apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

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


sudo apt update \
     apt install -y mysql-server \
     systemctl enable --now mysql

mysql -u root -p -e "CREATE DATABASE IF NOT EXISTS test_rest_DB;"
mysql -u root -p test_rest_DB -e "SHOW TABLES;"
mysql -u root -p -e "SHOW VARIABLES LIKE 'port';"

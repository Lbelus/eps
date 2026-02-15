FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and core libraries
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    clang \
    cmake \
    git \
    wget \
    unzip \
    pkg-config \
    libasio-dev \
    default-libmysqlclient-dev \ 
    libmysql++-dev \
    libhiredis-dev \
    curl && \
    apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Crow
RUN wget -O CrowCpp.zip https://github.com/CrowCpp/Crow/archive/refs/heads/master.zip && \
    unzip CrowCpp.zip && \
    cd Crow-master && \
    mkdir build && \
    cd build && \
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF && \
    make && \
    make install && \
    cd ../../ && \
    rm -rf CrowCpp.zip Crow-master

# Install redis-plus-plus
RUN wget -O redis-plus-plus.zip https://github.com/sewenew/redis-plus-plus/archive/refs/heads/master.zip && \
    unzip redis-plus-plus.zip && \
    cd redis-plus-plus-master && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install && \
    cd ../../ && \
    rm -rf redis-plus-plus.zip redis-plus-plus-master

# Install nlohmann/json
RUN git clone https://github.com/nlohmann/json.git /usr/src/nlohmann_json && \
    cd /usr/src/nlohmann_json && \
    cmake -S . -B build -DBUILD_TESTING=OFF && \
    cmake --install build && \
    rm -rf /usr/src/nlohmann_json

# (Optional) Install GoogleTest
RUN wget -O google-test.zip https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip && \
    unzip google-test.zip && \
    cd googletest-* && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install && \
    cd ../../ && \
    rm -rf google-test.zip googletest-*

ENV DEBIAN_FRONTEND=

WORKDIR /workspace

CMD ["bash"]


# default-libmysqlclient-dev is replacing libmysqlclient-dev


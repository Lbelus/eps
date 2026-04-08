FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive
# llvm is not mandatory 
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    llvm \
    libssl-dev \
    clang \
    cmake \
    curl \
    git \
    wget \
    unzip \
    pkg-config && \
    apt-get autoremove -y && \
    apt-get clean

# Install Node.js (LTS Version) and npm
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && \
    apt-get install -y nodejs && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Yarn
RUN npm install -g yarn

# Install supabase
# RUN npm install @supabase/supabase-js

EXPOSE 3000

ENV DEBIAN_FRONTEND=

WORKDIR /workspace
CMD ["/bin/bash"]

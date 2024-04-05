FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN echo "Acquire::http::Pipeline-Depth 0;" > /etc/apt/apt.conf.d/99custom && \
    echo "Acquire::http::No-Cache true;" >> /etc/apt/apt.conf.d/99custom && \
    echo "Acquire::BrokenProxy    true;" >> /etc/apt/apt.conf.d/99custom

ENV DEBIAN_FRONTEND=noninteractive

# Clean APT cache, update and install packages excluding libbz2-dev
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    apt-get update && \
    apt-get install -y \
    zlib1g-dev \
    libsnappy-dev \
    libzstd-dev \
    libgflags-dev \
    libbz2-dev \
    liblz4-dev \
    git \
    bash \
    make \
    g++

# Attempt to install libbz2-dev separately
RUN apt-get install -y 

# Clone and build RocksDB
RUN git clone https://github.com/facebook/rocksdb.git && \
    cd rocksdb/ && \
    make static_lib && \
    make install-static INSTALL_PATH=/usr/local


# Install FUSE
RUN apt-get install -y \
    fuse \
    libfuse-dev

RUN ls /usr/local/include/rocksdb  # Check if RocksDB headers are present
RUN ls /usr/local/lib/*rocksdb*    # Check if RocksDB library files are present

CMD ["make", "run"]



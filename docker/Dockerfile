FROM registry.astralinux.ru/library/astra/ubi18-cpp122:latest AS builder

RUN apt-get update && apt-get install -y \
    git \
    libmemcached-dev \
    libpq-dev \
    libevent-dev \
    pkg-config \
    cmake \
    make \
    g++ \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth=1 https://github.com/SergiusTheBest/plog.git /tmp/plog && \
    mv /tmp/plog/include/plog /usr/local/include/plog && rm -rf /tmp/plog

RUN git clone --branch 7.7.0 https://github.com/jtv/libpqxx.git /tmp/libpqxx && \
    cd /tmp/libpqxx && mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_DOC=OFF -DBUILD_TEST=OFF && \
    make -j$(nproc) && make install && cd / && rm -rf /tmp/libpqxx

WORKDIR /app
COPY CMakeLists.txt ./
COPY src/ ./src/

RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc)

FROM registry.astralinux.ru/library/astra/ubi18:latest

RUN apt-get update && apt-get install -y \
    libmemcached11 \
    libpq5 \
    libevent-2.1-7 \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/* && ldconfig

COPY --from=builder /app/build/memcached_writer /usr/local/bin/
COPY --from=builder /app/build/memcached_to_pg /usr/local/bin/

WORKDIR /app
CMD ["/bin/bash"]
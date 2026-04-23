FROM registry.astralinux.ru/library/astra/ubi18:latest AS builder

RUN apt update && apt install -y \
    g++ \
    cmake \
    make \
    git \
    libmemcached-dev \
    libpq-dev \
    libevent-dev \
    pkg-config \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
RUN git clone --branch 7.7.0 https://github.com/jtv/libpqxx.git && \
    cd libpqxx && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_DOC=OFF -DBUILD_SHARED_LIBS=ON && \
    make -j$(nproc) && \
    make install && \
    cd /tmp && rm -rf libpqxx

WORKDIR /app
COPY memc_writer.cpp ./
COPY pg_memc.cpp ./
COPY third_party/plog ./third_party/plog
COPY CMakeLists.txt ./

RUN mkdir build && cd build && cmake .. && make

FROM registry.astralinux.ru/library/astra/ubi18:latest

RUN apt update && apt install -y \
    libpq5 \
    libevent-2.1-7 \
    libmemcached-tools \
    postgresql-client \
    coreutils \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/lib/x86_64-linux-gnu/libmemcached* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libhashkit* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libpq* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libevent* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libpqxx* /usr/lib/x86_64-linux-gnu/

COPY --from=builder /app/build/memcached_writer /usr/local/bin/
COPY --from=builder /app/build/memcached_to_pg /usr/local/bin/
COPY run_test.sh /app/run_test.sh
RUN chmod +x /app/run_test.sh

RUN ldconfig

CMD ["/bin/bash"]
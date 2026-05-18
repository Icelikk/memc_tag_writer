#include <iostream>
#include <libmemcached/memcached.h>
#include <pqxx/pqxx>
#include <chrono>
#include <string>
#include <vector>
#include <cstring> 

int main() {
    memcached_st *memc = memcached_create(NULL);
    memcached_server_st *servers = memcached_server_list_append(NULL, "memcached", 11211, NULL);
    memcached_server_push(memc, servers);
    memcached_server_list_free(servers);

    pqxx::connection conn("host=postgres-memc port=5432 dbname=test user=postgres password=postgres");
    if (!conn.is_open()) {
        std::cerr << "[ERROR] PostgreSQL connection failed\n";
        memcached_free(memc);
        return 1;
    }

    {
        pqxx::work setup(conn);
        setup.exec("SET synchronous_commit = OFF");
        setup.exec("SET temp_buffers = '256MB'");
        setup.exec("SET maintenance_work_mem = '256MB'");
        setup.commit();
    }

    auto start_time = std::chrono::steady_clock::now();
    size_t total_processed = 0;
    const size_t BATCH_SIZE = 5000;
    std::vector<std::string> batch;
    batch.reserve(BATCH_SIZE);

    int packet_id = 1;
    while (true) {
        std::string key = "batch_list:" + std::to_string(packet_id);
        size_t value_length;
        uint32_t flags;
        memcached_return_t rc;

        char *raw_value = memcached_get(memc, key.c_str(), key.size(), &value_length, &flags, &rc);

        if (rc == MEMCACHED_NOTFOUND) {
            std::cout << "[INFO] Ключ " << key << " не найден. Завершаю выгрузку.\n";
            break;
        } else if (rc != MEMCACHED_SUCCESS) {
            std::cerr << "[WARN] Ошибка чтения " << key << ": " << memcached_strerror(memc, rc) << "\n";
            packet_id++;
            continue;
        }

        std::string blob(raw_value, value_length);
        free(raw_value);

        const char* data = blob.data();
        size_t pos = 0;
        size_t len = blob.size();

        while (pos < len) {
            size_t next_nl = blob.find('\n', pos);
            if (next_nl == std::string::npos) next_nl = len;
            size_t line_len = next_nl - pos;
            if (line_len == 0) { pos = next_nl + 1; continue; }

            const char* line_start = data + pos;
            const char* p1 = (const char*)std::memchr(line_start, ',', line_len);
            const char* p2 = p1 ? (const char*)std::memchr(p1 + 1, ',', line_len - (p1 - line_start) - 1) : nullptr;
            const char* p3 = p2 ? (const char*)std::memchr(p2 + 1, ',', line_len - (p2 - line_start) - 1) : nullptr;

            if (p1 && p2 && p3) {
                size_t l_id = p1 - line_start;
                size_t l_q  = p2 - p1 - 1;
                size_t l_v  = p3 - p2 - 1;
                size_t l_t  = line_len - (p3 - line_start) - 1;

                std::string row;
                row.reserve(l_id + l_q + l_v + l_t + 3);
                row.append(line_start, l_id); row += '\t';
                row.append(p1 + 1, l_q);      row += '\t';
                row.append(p2 + 1, l_v);      row += '\t';
                row.append(p3 + 1, l_t);

                batch.push_back(std::move(row));
            }

            pos = next_nl + 1;

            if (batch.size() >= BATCH_SIZE) {
                try {
                    pqxx::work txn(conn);
                    auto stream = pqxx::stream_to::table(txn, {"guts"}, {"id", "q_val", "v_val", "t"});
                    for (const auto& r : batch) stream << r;
                    stream.complete();
                    txn.commit();
                    total_processed += batch.size();
                } catch (const std::exception &e) {
                    std::cerr << "[ERROR] PG batch failed: " << e.what() << "\n";
                }
                batch.clear();
            }
        }

        memcached_delete(memc, key.c_str(), key.size(), 0);
        packet_id++;
    }

    if (!batch.empty()) {
        try {
            pqxx::work txn(conn);
            auto stream = pqxx::stream_to::table(txn, {"guts"}, {"id", "q_val", "v_val", "t"});
            for (const auto& r : batch) stream << r;
            stream.complete();
            txn.commit();
            total_processed += batch.size();
        } catch (const std::exception &e) {
            std::cerr << "[ERROR] PG batch failed: " << e.what() << "\n";
        }
    }

    memcached_free(memc);
    auto end_time = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Processed " << total_processed << " records in " << dur << " ms\n";
    return 0;
}
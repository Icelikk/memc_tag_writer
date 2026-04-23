#include <libmemcached/memcached.h>
#include <pqxx/pqxx>
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <num_packets>" << std::endl;
        return 1;
    }
    int num_packets = std::stoi(argv[1]);

    
    memcached_st *memc = memcached_create(NULL);
    memcached_server_st *servers = memcached_server_list_append(NULL, "memcached", 11211, NULL);
    memcached_server_push(memc, servers);
    memcached_server_list_free(servers);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 0);

   
    pqxx::connection conn("host=postgres port=5432 dbname=test user=postgres password=postgres");
    if (!conn.is_open()) {
        std::cerr << "Failed to connect to PostgreSQL: " << conn.dbname() << std::endl;
        memcached_free(memc);
        return 1;
    }

    auto start = std::chrono::steady_clock::now();

    const size_t BATCH_SIZE = 1000;
    std::vector<std::string> buffer;
    buffer.reserve(BATCH_SIZE);

    auto flush_buffer = [&]() {
        if (buffer.empty()) return;
        std::ostringstream sql;
        sql << "INSERT INTO guts (id, q_val, v_val, t) VALUES ";
        bool first = true;
        for (const auto& rec_str : buffer) {
            if (!first) sql << ",";
            first = false;
            sql << "(" << rec_str << ")";
        }
        sql << ";";
        pqxx::work txn(conn);
        txn.exec(sql.str());
        txn.commit();
        buffer.clear();
    };

    int total_records = 0;
    for (int i = 1; i <= num_packets; ++i) {
        std::string key = "batch_list:" + std::to_string(i);
        size_t value_len;
        uint32_t flags;
        memcached_return_t rc;
        char* value = memcached_get(memc, key.c_str(), key.size(), &value_len, &flags, &rc);
        if (rc == MEMCACHED_SUCCESS) {
            std::string data(value, value_len);
            std::istringstream iss(data);
            std::string line;
            while (std::getline(iss, line)) {
                if (!line.empty()) {
                    buffer.push_back(line);
                    total_records++;
                    if (buffer.size() >= BATCH_SIZE) {
                        flush_buffer();
                    }
                }
            }
            free(value);
        } else {
            std::cerr << "Key " << key << " not found" << std::endl;
        }
    }
    flush_buffer();

    memcached_free(memc);

    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Processed " << total_records << " records from " << num_packets << " packets." << std::endl;
    std::cout << "Total time (read from Memcached + insert to PG): " << duration_ms << " ms" << std::endl;
    
    return 0;
}

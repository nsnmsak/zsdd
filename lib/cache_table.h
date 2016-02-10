#ifndef CACHE_TABLE_H_
#define CACHE_TABLE_H_
#include <vector>
#include <tuple>
#include <iostream>
#include <algorithm>
// #include "zsdd_common.h"
#include "zsdd_node.h"

namespace zsdd {

class CacheTable {
    using cache_entry = std::tuple<Operation, addr_t, addr_t, addr_t>;
public:

    CacheTable() : cache_table_(INIT_SIZE) {}
    CacheTable(const int init_size) : cache_table_(init_size) {}    

    void write_cache(const Operation op, const addr_t lhs, 
                     const addr_t rhs, const addr_t res) {
        auto key = calc_key(op, lhs, rhs);
        cache_table_[key] = std::make_tuple(op, lhs, rhs, res);
    }


    void clear_cache() {
        for (auto it =  cache_table_.begin(); it != cache_table_.end(); ++it) {
            *it = std::make_tuple(Operation::NULLOP, -1, -1, -1);
        }
    }

    addr_t read_cache(const Operation op, const addr_t lhs, const addr_t rhs) {
        auto key = calc_key(op, lhs, rhs);
        auto res = cache_table_[key];
        
        if (std::get<0>(res) == op &&
            std::get<1>(res) == lhs &&
            std::get<2>(res) == rhs) {
            return std::get<3>(res);
        }
        return ZSDD_NULL;
    }



    void extend_table() {
        const auto prev_size = cache_table_.size();
        cache_table_.reserve(cache_table_.size() << TABLE_EXTEND_FACTOR);
        for (size_t k = 1; k < (1U << TABLE_EXTEND_FACTOR); k++) {
            for (size_t i = 0; i < prev_size; i++) {
                cache_table_.push_back(cache_table_[i]);
            }
        }
    }    

private:
    const unsigned int TABLE_EXTEND_FACTOR = 2;
    const unsigned int INIT_SIZE = 1U<<8;
    std::vector<cache_entry> cache_table_;

    size_t calc_key(const Operation op, const addr_t lhs,  const addr_t rhs) {
        size_t key = 0;
        hash_combine(key, std::hash<int>()(static_cast<int>(op)));
        hash_combine(key, std::hash<size_t>()(lhs));
        hash_combine(key, std::hash<size_t>()(rhs));
        return key % cache_table_.size();
    }

};
} // namespace zsdd;
#endif //CACHE_TABLE_H_

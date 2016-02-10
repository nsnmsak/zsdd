#ifndef ZSDD_H_
#define ZSDD_H_

#include "zsdd_manager.h"
#include <iostream>

namespace zsdd {

class Zsdd {
public:
    Zsdd(const addr_t addr, ZsddManager& manager) : 
        addr_(addr), mngr_(manager) { 
        mngr_.inc_zsddnode_refcount_at(addr_);
    } 
    Zsdd(const Zsdd& obj) : 
        addr_(obj.addr_), mngr_(obj.mngr_) {
        mngr_.inc_zsddnode_refcount_at(addr_);
    }
    Zsdd(const Zsdd&& obj) : 
        addr_(obj.addr_), mngr_(obj.mngr_) {
        mngr_.inc_zsddnode_refcount_at(addr_);
    }
    ~Zsdd() {
        mngr_.dec_zsddnode_refcount_at(addr_);
    }
    Zsdd& operator=(const Zsdd& obj) {
        if (addr_ == obj.addr_) return *this;
        mngr_.dec_zsddnode_refcount_at(addr_);
        addr_ = obj.addr_;
        mngr_.inc_zsddnode_refcount_at(addr_);
        return *this;
    }
    unsigned long long int count_solution() const {
        return mngr_.count_solution(addr_);
    }

    unsigned long long int size() const {
        return mngr_.size(addr_);
    }
    addr_t addr() const { return addr_; }

    void export_txt(std::ostream& os) const {
        mngr_.export_zsdd_txt(addr_, os);
    }
    void export_dot(std::ostream& os) const {
        mngr_.export_zsdd_dot(addr_, os);
    }

private:
    addr_t addr_;
    ZsddManager& mngr_;
};
}

#endif // ZSDD_H_

#ifndef ZSDD_NODE_H_
#define ZSDD_NODE_H_
#include <vector>
#include <iostream>
#include "zsdd_common.h"
#include "zsdd_vtree.h"
#include <unordered_set>
#include <type_traits>
#include <functional>

namespace zsdd {

class ZsddManager;

using ZsddElement = std::pair<addr_t, addr_t>;

class ZsddNode {
public:
    ZsddNode() : 
        type_(NodeType::UNUSED), 
        literal_(0), 
        decomp_(),
        vtree_node_id_(-1), 
        refcount_(0) {}
    
    ZsddNode(const int literal, const int vtree_node_id) :
        type_(NodeType::LIT),
        literal_(literal),
        decomp_(),
        vtree_node_id_(vtree_node_id),
        refcount_(0) {}
    
    ZsddNode(const std::vector<ZsddElement>& decomp,
             const int vtree_node_id) :
        type_(NodeType::DECOMP),
        literal_(0),
        decomp_(decomp),
        vtree_node_id_(vtree_node_id),
        refcount_(0) {}

    ZsddNode(const std::vector<ZsddElement>&& decomp,
             const int vtree_node_id) :
        type_(NodeType::DECOMP),
        literal_(0),
        decomp_(std::move(decomp)),
        vtree_node_id_(vtree_node_id),
        refcount_(0) {}
    
    ZsddNode(const ZsddNode& obj) :
        type_(obj.type_), 
        literal_(obj.literal_),
        decomp_(obj.decomp_),
        vtree_node_id_(obj.vtree_node_id_),
        refcount_(obj.refcount_) {}

    ZsddNode(ZsddNode&& obj) :
        type_(obj.type_), 
        literal_(obj.literal_),
        decomp_(std::move(obj.decomp_)),
        vtree_node_id_(obj.vtree_node_id_), 
        refcount_(obj.refcount_) {}

    // set the node as Unused
    // (appear only in cache.
    void deactivate() {
        type_ = NodeType::UNUSED;
        literal_ = -1;
        decomp_.clear();
        vtree_node_id_ = -1;
        refcount_ = 0;
    }

    void operator=(const ZsddNode& obj)  = delete;

    // set unused not to the value of obj.
    void activate(ZsddNode&& obj) {
        assert(type_ == NodeType::UNUSED);
        type_ = obj.type_;
        vtree_node_id_ = obj.vtree_node_id_;
        literal_ = obj.literal_;            
        decomp_ = std::move(obj.decomp_);
        refcount_ = obj.refcount_;
    }
    
    bool operator==(const ZsddNode& n) const {
        if (type_ != n.type_) return false;
        if (type_ == NodeType::LIT) {
            return vtree_node_id_ == n.vtree_node_id_ &&
                literal_ == n.literal_;
        } else {
            return vtree_node_id_ == n.vtree_node_id_ &&
                decomp_ == n.decomp_;
        }
    }

    NodeType type() const { return type_; }
    int literal() const { return literal_; }
    const std::vector<ZsddElement>& decomposition() const { return decomp_; }
    int vtree_node_id() const { return vtree_node_id_; }

    unsigned int refcount() const { return refcount_; }

    // increment/decrement reference counter.
    // the reference counter is used in gc().
    void inc_ref_count(ZsddManager& mgr);
    void dec_ref_count(ZsddManager& mgr);

private:
    NodeType type_; // nodetype
    int literal_; // literal value if node respects to leaf vtree.
    std::vector<ZsddElement> decomp_; // decomposition(represented as pairs of zsddnoes)

    int vtree_node_id_;
    unsigned int refcount_;
};


} // namespace zsdd

namespace std {
template <> struct hash<zsdd::ZsddNode> {
    std::size_t operator()(const zsdd::ZsddNode& n) const {
        size_t h = 0;
        if (n.type() == zsdd::NodeType::LIT) {
            zsdd::hash_combine(h, hash<int>()(n.literal()));
            zsdd::hash_combine(h, hash<int>()(n.vtree_node_id()));
            return h;
        } else if (n.type() == zsdd::NodeType::DECOMP) {
            for (const auto& e : n.decomposition()) {
                zsdd::hash_combine(h, hash<zsdd::addr_t>()(e.first));
                zsdd::hash_combine(h, hash<zsdd::addr_t>()(e.second));
            }
            zsdd::hash_combine(h, hash<int>()(n.vtree_node_id()));
        } 
        return h;
    }
};
}


#endif // ZSDD_NODE_H_

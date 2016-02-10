#include "zsdd_node.h"
#include "zsdd_manager.h"
#include <assert.h>
#include <sstream>

namespace zsdd {


void ZsddNode::inc_ref_count(ZsddManager& mgr) {
    if (type_ == NodeType::LIT) return;

    refcount_++;
    if (refcount_ == 1) {
        for (const auto& e : decomp_) {
            if (e.first >= 0) {
                ZsddNode& p = mgr.get_zsddnode_at(e.first);
                p.inc_ref_count(mgr);
            }
            if (e.second >= 0) {
                ZsddNode& s = mgr.get_zsddnode_at(e.second);
                s.inc_ref_count(mgr);
            }
        }
    }
}

void ZsddNode::dec_ref_count(ZsddManager& mgr) {
    if (type_ == NodeType::LIT) return;

    refcount_--;
    if (refcount_ == 0) {
        for (const auto& e : decomp_) {
            if (e.first >= 0) {
                ZsddNode& p = mgr.get_zsddnode_at(e.first);
                p.dec_ref_count(mgr);
            }
            if (e.second >= 0) {
                ZsddNode& s = mgr.get_zsddnode_at(e.second);
                s.dec_ref_count(mgr);
            }
        }
    }
}


} // namespace zsdd;

#ifndef ZSDD_MANAGER_H_
#define ZSDD_MANAGER_H_
#include <functional>
#include <vector>
#include <stack>
#include <unordered_map>
#include "zsdd_common.h"
#include "zsdd_node.h"
#include "zsdd_vtree.h"
#include "zsdd_nodetable.h"
#include "cache_table.h"



namespace zsdd {

class Zsdd;

class ZsddManager {
public:
    ZsddManager(const VTree& vtree, const unsigned int cache_size = 1U << 16) 
        : vtree_(vtree), 
          cache_table_(cache_size),
          zsdd_node_table_()
        {}


    // Apply operations
    Zsdd zsdd_intersection(const Zsdd& lhs, const Zsdd& rhs);
    Zsdd zsdd_union(const Zsdd& lhs, const Zsdd& rhs);
    Zsdd zsdd_difference(const Zsdd& lhs, const Zsdd& rhs);
    Zsdd zsdd_orthogonal_join(const Zsdd& lhs, const Zsdd& rhs);
    
    // Apply operations with a variable.
    Zsdd zsdd_change(const Zsdd& zsdd, const addr_t var);
    Zsdd zsdd_filter_contain(const Zsdd& zsdd, const addr_t var);
    Zsdd zsdd_filter_not_contain(const Zsdd& zsdd, const addr_t var);
    
    // Restore implicit partitions.
    Zsdd zsdd_to_explicit_form(const Zsdd& zsdd);

    // increment reference counter
    void inc_zsddnode_refcount_at(const addr_t idx) {
        if (idx < 0) return;
        zsdd_node_table_.get_node_at(idx).inc_ref_count(*this);
    }
    // decrement refcount
    void dec_zsddnode_refcount_at(const addr_t idx) {
        if (idx < 0) return;
        zsdd_node_table_.get_node_at(idx).dec_ref_count(*this);
    }
    
    // garbage collection 
    // delete nodes whose refcount is 0.
    void gc();


    ZsddNode& get_zsddnode_at(const addr_t idx)  {
        return zsdd_node_table_.get_node_at(idx);
    }
    const ZsddNode& get_zsddnode_at(const addr_t idx) const {
        return zsdd_node_table_.get_node_at(idx);
    }


    


    // make terminal/variable zsdds.
    Zsdd make_zsdd_literal(const addr_t literal);
    Zsdd make_zsdd_baseset();
    Zsdd make_zsdd_empty();

    // make zsdd representing power set whose base set 
    // consists of leaves of vtree_node.
    Zsdd make_zsdd_powerset(const int vtree_node);

    // model counting
    unsigned long long count_solution(const addr_t zsdd) const;

    // size of zsdds.
    unsigned long long size(const addr_t zsdd) const;
    std::vector<std::vector<int>> calc_setfamily(const addr_t zsdd) const;


    void export_zsdd_txt(const addr_t zsdd, std::ostream& os) const;
    void export_zsdd_dot(const addr_t zsdd, std::ostream& os) const;

private:
    addr_t make_zsdd_literal_inner(const addr_t literal);
    addr_t zsdd_to_explicit_form_inner(const addr_t zsdd);
    std::vector<ZsddElement> compress_candidates(const std::vector<std::pair<addr_t, addr_t>>&
                                                  new_decomp_candidates);

    addr_t zsdd_apply_withvar(const Operation& op, const addr_t zsdd, const addr_t var);


    addr_t make_zsdd_decomposition(std::vector<ZsddElement>&& decomp_nodes, const int vtree_node);
    addr_t calc_primes_union(const std::vector<ZsddElement>& decomp);
    addr_t make_zsdd_powerset_inner(const int vtree_node);

    unsigned long long count_solution_inner(const addr_t zsdd, std::unordered_map<addr_t, unsigned long long>& cache) const;
    std::vector<std::vector<int>> calc_setfamily_inner(const addr_t zsdd, std::unordered_map<addr_t, std::vector<std::vector<int>>>& cache) const; 

    void export_zsdd_txt_inner(const addr_t zsdd, std::ostream& os, 
                               std::unordered_set<addr_t>& found, 
                               const size_t empty_id, 
                               const size_t false_id) const;


    addr_t zsdd_apply(const Operation& op, const addr_t lhs, const addr_t rhs);

    VTree vtree_;
    CacheTable cache_table_;
    ZsddNodeTable zsdd_node_table_;

};

}
#endif // ZSDD_MANAGER_H_

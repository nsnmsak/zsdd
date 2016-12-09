#include "zsdd_manager.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include "zsdd.h"

namespace zsdd {

Zsdd ZsddManager::make_zsdd_baseset() {
    return Zsdd(ZSDD_EMPTY, *this); 
}


Zsdd ZsddManager::make_zsdd_literal(const addr_t literal) {
    addr_t res = make_zsdd_literal_inner(literal);
    return Zsdd(res, *this);
}


addr_t ZsddManager::make_zsdd_literal_inner(const addr_t literal) {
    const int v_id = vtree_.find_literal_node_id(literal);
    return zsdd_node_table_.make_or_find_literal(literal, v_id);
}


Zsdd ZsddManager::make_zsdd_empty() {
    return Zsdd(ZSDD_FALSE, *this); 
}


void ZsddManager::gc() {
    zsdd_node_table_.gc();
    cache_table_.clear_cache();
}


Zsdd ZsddManager::zsdd_intersection(const Zsdd& lhs, const Zsdd& rhs) {
    addr_t res =  zsdd_apply(Operation::INTERSECTION, lhs.addr(), rhs.addr());
    return Zsdd(res, *this);
}


Zsdd ZsddManager::zsdd_union(const Zsdd& lhs, const Zsdd& rhs) {
    addr_t res =  zsdd_apply(Operation::UNION, lhs.addr(), rhs.addr());
    return Zsdd(res, *this);
}


Zsdd ZsddManager::zsdd_difference(const Zsdd& lhs, const Zsdd& rhs) {
    addr_t res =  zsdd_apply(Operation::DIFFERENCE, lhs.addr(), rhs.addr());
    return Zsdd(res, *this);
}

Zsdd ZsddManager::zsdd_orthogonal_join(const Zsdd& lhs, const Zsdd& rhs) {
    addr_t res =  zsdd_apply(Operation::ORTHOGONAL_JOIN, lhs.addr(), rhs.addr());
    return Zsdd(res, *this);
}


addr_t ZsddManager::make_zsdd_decomposition(std::vector<ZsddElement>&& decomp_nodes, 
                                            const int vtree_node) {
    assert(!decomp_nodes.empty());
    return zsdd_node_table_.make_or_find_decomp(std::move(decomp_nodes), vtree_node);
}


addr_t ZsddManager::calc_primes_union(const std::vector<ZsddElement>& decomp) {
    addr_t primes_union = ZSDD_FALSE;
    for (auto elem : decomp) {
        addr_t prev = primes_union;
        primes_union = zsdd_apply(Operation::UNION, elem.first, prev);
    }
    return primes_union;
}


Zsdd ZsddManager::zsdd_change(const Zsdd& z, const addr_t var) {
    addr_t res = zsdd_apply_withvar(Operation::CHANGE, z.addr(), var);
    return Zsdd(res, *this);
}


Zsdd ZsddManager::zsdd_filter_contain(const Zsdd& z, const addr_t var) {
    addr_t res = zsdd_apply_withvar(Operation::FILTER_CONTAIN, z.addr(), var);
    return Zsdd(res, *this);
}


Zsdd ZsddManager::zsdd_filter_not_contain(const Zsdd& z, const addr_t var) {
    addr_t res = zsdd_apply_withvar(Operation::FILTER_NOT_CONTAIN, z.addr(), var);
    return Zsdd(res, *this);
}


addr_t ZsddManager::zsdd_apply_withvar(const Operation& op, const addr_t zsdd, const addr_t var) {
    if (op != Operation::CHANGE &&
        op != Operation::FILTER_CONTAIN &&
        op != Operation::FILTER_NOT_CONTAIN) {
        std::cerr << "[error] unsupported operation on zsdd_apply_withvar" << std::endl;
        exit(1);
    }
    
    if (zsdd == ZSDD_FALSE || zsdd == ZSDD_NULL) {
        return zsdd;
    }
    if (zsdd == ZSDD_EMPTY) {
        if (op == Operation::CHANGE) {
            return make_zsdd_literal_inner(var);            
        } 
        else if (op == Operation::FILTER_CONTAIN) {
            return ZSDD_FALSE;
        }
        else if (op == Operation::FILTER_NOT_CONTAIN) {
            return ZSDD_EMPTY;
        }

    }

    const ZsddNode n = get_zsddnode_at(zsdd);
    if (n.type() == NodeType::LIT) {
        if (op == Operation::CHANGE) {
            if (llabs(n.literal()) == var) {
                if (n.literal() < 0) {
                    return zsdd;
                } else {
                    return ZSDD_EMPTY;
                }
            }
        }
        else if (op == Operation::FILTER_CONTAIN) {
            if (llabs(n.literal()) == var) {
                if (n.literal() < 0) {
                    return make_zsdd_literal_inner(var);
                } else {
                    return zsdd;
                }
            }
        }
        else if (op == Operation::FILTER_NOT_CONTAIN) {
            if (llabs(n.literal()) == var) {
                if (n.literal() < 0) {
                    return ZSDD_EMPTY;
                } else {
                    return ZSDD_FALSE;
                }
            }
        }
    }        

    {
        addr_t cache = cache_table_.read_cache(op, zsdd, var);
        if (cache != ZSDD_NULL) {
            return cache;
        }
    }

    const int var_vtree_id = vtree_.find_literal_node_id(var);
    const int depend_vtree_id = vtree_.get_depend_node(n.vtree_node_id(), 
                                                       var_vtree_id);

    if (depend_vtree_id == n.vtree_node_id()) {
        std::vector<std::pair<addr_t, addr_t>> candidates;
        auto& decomp = n.decomposition();
        assert(!decomp.empty());
        if (vtree_.is_left_descendant(depend_vtree_id,  var_vtree_id)) {
            for (const auto& e : decomp) {
                addr_t new_p = zsdd_apply_withvar(op, e.first, var);

                if (new_p == ZSDD_FALSE) continue;                    
                candidates.emplace_back(new_p, e.second);
            }
        } else {
            for (const auto e : decomp) {
                addr_t new_s = zsdd_apply_withvar(op, e.second, var);
                if (new_s == ZSDD_FALSE) continue;
                candidates.emplace_back(e.first, new_s);
            }
        }
        if (candidates.empty()) {
            cache_table_.write_cache(op, zsdd, var, ZSDD_FALSE);
            return ZSDD_FALSE;
        }
        std::vector<ZsddElement> new_decomposition = compress_candidates(candidates);

        // zero suppression        
        if (new_decomposition.size() == 1) {
            ZsddElement& e = new_decomposition[0];
            if (e.first == ZSDD_EMPTY) {
                cache_table_.write_cache(op, zsdd, var, e.second);
                return e.second;
            } 
            if (e.second == ZSDD_EMPTY) {
                cache_table_.write_cache(op, zsdd, var, e.first);
                return e.first;
            }            
        }

        addr_t new_res = make_zsdd_decomposition(std::move(new_decomposition), 
                                                 depend_vtree_id);
        cache_table_.write_cache(op, zsdd, var, new_res);
        return new_res;
    } 
    else { // depend_vtree_id != n.vtree_node_id
        
        if (op == Operation::CHANGE) {
            std::vector<ZsddElement> new_decomp;
            if (vtree_.is_left_descendant(depend_vtree_id, var_vtree_id)) {
                addr_t new_prime = make_zsdd_literal_inner(var);
                new_decomp.emplace_back(new_prime, zsdd);
            } else {
                addr_t new_sub = make_zsdd_literal_inner(var);
                new_decomp.emplace_back(zsdd, new_sub);
            }
            addr_t new_res = make_zsdd_decomposition(std::move(new_decomp), depend_vtree_id);
            cache_table_.write_cache(op, zsdd, var, new_res);
            return new_res;
        }
        else if (op == Operation::FILTER_CONTAIN) {
            cache_table_.write_cache(op, zsdd, var, ZSDD_FALSE);
            return ZSDD_FALSE;
        }
        else if (op == Operation::FILTER_NOT_CONTAIN) {
            cache_table_.write_cache(op, zsdd, var, zsdd);
            return zsdd;
        }
        else {
            std::cerr << "[error] invalid operation" << std::endl;
            exit(1);
        }
    }
}


addr_t ZsddManager::zsdd_apply(const Operation& op, const addr_t lhs, const addr_t rhs) {
    if (op == Operation::INTERSECTION || 
        op == Operation::UNION || 
        op == Operation::ORTHOGONAL_JOIN) {
        if (lhs > rhs) return zsdd_apply(op, rhs, lhs);
    }
    // check trivial case
    if (op ==  Operation::INTERSECTION) {
        if (lhs == ZSDD_NULL || rhs == ZSDD_NULL) return ZSDD_NULL;
        if (lhs == ZSDD_FALSE || rhs == ZSDD_FALSE) return ZSDD_FALSE;
        if (lhs == ZSDD_EMPTY && rhs == ZSDD_EMPTY) return ZSDD_EMPTY;

        if (lhs == rhs) {
            return lhs;
        }
        // since rhs > lhs, we check only lhs.
        const ZsddNode& r_node = get_zsddnode_at(rhs);        
        if (lhs == ZSDD_EMPTY && r_node.type() == NodeType::LIT) {
            if (r_node.literal() < 0) {
                return ZSDD_EMPTY;
            } else {
                return ZSDD_FALSE;
            }
        } 
        if (lhs >= 0) {
            const ZsddNode& l_node = get_zsddnode_at(lhs);
            if (l_node.type() == NodeType::LIT && r_node.type() == NodeType::LIT) {
                if (llabs(l_node.literal()) == llabs(r_node.literal())) {
                    if (l_node.literal() > 0) {
                        return lhs;
                    } else {
                        return rhs;
                    }
                } else if (l_node.literal() < 0 && r_node.literal() < 0) {
                    return ZSDD_EMPTY;
                } else {
                    return ZSDD_FALSE;
                }
            }
        }
    } 
    else if (op == Operation::UNION) {
        if (lhs == ZSDD_NULL || rhs == ZSDD_NULL) return ZSDD_NULL;
        if (lhs == ZSDD_EMPTY && rhs == ZSDD_EMPTY) return ZSDD_EMPTY;        
        if (lhs == ZSDD_FALSE) {
            return rhs;
        }
        if (rhs == ZSDD_FALSE) {
            return lhs;
        }

        if (lhs == rhs) {
            return lhs;
        }
        // since rhs > lhs, rhs is always >= 0
        const ZsddNode& r_node = get_zsddnode_at(rhs);        
        if (lhs == ZSDD_EMPTY && r_node.type() == NodeType::LIT) {
            if (r_node.literal() < 0) {
                return rhs;
            } else {
                addr_t l =  make_zsdd_literal_inner(-r_node.literal());
                return l;
            }
        }
        if (lhs >= 0) {
            const ZsddNode& l_node = get_zsddnode_at(lhs);
            if (l_node.type() == NodeType::LIT && r_node.type() == NodeType::LIT) {
                if (llabs(l_node.literal()) == llabs(r_node.literal())) {
                    if (l_node.literal() < 0) {
                        return lhs;
                    } else {
                        return rhs;
                    }
                }
            }
        }
        
    }
    else if (op == Operation::DIFFERENCE) {
        if (lhs == ZSDD_NULL || rhs == ZSDD_NULL) return ZSDD_NULL;
        if (lhs == ZSDD_FALSE) return ZSDD_FALSE;
        if (rhs == ZSDD_FALSE) {
            return lhs;
        }
        if (lhs == rhs) return ZSDD_FALSE;
        if (lhs == ZSDD_EMPTY || rhs == ZSDD_EMPTY) {
            if (lhs >= 0) {
                const ZsddNode& node = get_zsddnode_at(lhs);
                if (node.type() == NodeType::LIT) {
                    if (node.literal() < 0) {
                        return make_zsdd_literal_inner(-node.literal());
                    } else {
                        return lhs;
                    }
                }
            } 
            else { // rhs >= 0
                const ZsddNode& node = get_zsddnode_at(rhs);
                if (node.type() == NodeType::LIT) {
                    if (node.literal() < 0) {
                        return ZSDD_FALSE;
                    } else {
                        return ZSDD_EMPTY;
                    }
                }
            }
        }
        if (lhs >= 0 && rhs >= 0) {
            const ZsddNode& l_node = get_zsddnode_at(lhs);
            const ZsddNode& r_node = get_zsddnode_at(rhs);
            if (l_node.type() == NodeType::LIT && r_node.type() == NodeType::LIT) {
                addr_t l_lit = l_node.literal();
                addr_t r_lit = r_node.literal();
                if (llabs(l_lit) == llabs(r_lit)) {
                    if (l_node.literal() > 0) {
                        return ZSDD_FALSE;
                    } else {
                        return ZSDD_EMPTY;
                    }
                } else if (l_lit > 0) {
                    return lhs;
                } else {
                    if (r_lit < 0) {
                        return make_zsdd_literal_inner(-l_lit);
                    } else {
                        return lhs;
                    }
                }
            }
        }
    } 
    else if (op == Operation::ORTHOGONAL_JOIN) {
        if (lhs == ZSDD_NULL || rhs == ZSDD_NULL) return ZSDD_NULL;
        if (lhs == ZSDD_FALSE || rhs == ZSDD_FALSE) return ZSDD_FALSE;
        if (lhs == ZSDD_EMPTY) {
            return rhs;
        }
        if (rhs == ZSDD_EMPTY) {
            return rhs;
        }
        const ZsddNode& l_node = get_zsddnode_at(lhs);
        const ZsddNode& r_node = get_zsddnode_at(rhs);
        if (l_node.type() == NodeType::LIT &&
            r_node.type() == NodeType::LIT) {
            if (llabs(l_node.literal()) == llabs(r_node.literal())) {
                return ZSDD_FALSE; // must be simple join
            }
        }
    }

    // cache check
    {
        addr_t cache = cache_table_.read_cache(op, lhs, rhs);
        if (cache != ZSDD_NULL) {
            return cache;
        }
    }

    // setup decomposition nodes;
    std::vector<ZsddElement> decomp_l;
    std::vector<ZsddElement> decomp_r;
    addr_t depend_vtree_node_id;
    if (lhs < 0) {
        assert(lhs == ZSDD_EMPTY);

        const ZsddNode& n = get_zsddnode_at(rhs);
        depend_vtree_node_id = n.vtree_node_id();
        decomp_l = {{ZSDD_EMPTY, ZSDD_EMPTY}};
        decomp_r = n.decomposition();
    } 
    else if (rhs < 0) {
        assert(rhs == ZSDD_EMPTY);
        
        const ZsddNode& n = get_zsddnode_at(lhs);
        depend_vtree_node_id = n.vtree_node_id();
        decomp_l = n.decomposition();
        decomp_r = {{ZSDD_EMPTY, ZSDD_EMPTY}};
    }
    else {
        const ZsddNode& l_node = get_zsddnode_at(lhs);
        const ZsddNode& r_node = get_zsddnode_at(rhs);

        const addr_t l_vnode = l_node.vtree_node_id();
        const addr_t r_vnode = r_node.vtree_node_id();
        depend_vtree_node_id = vtree_.get_depend_node(l_vnode, r_vnode);
        if (l_vnode == r_vnode) {
            decomp_l = l_node.decomposition();
            decomp_r = r_node.decomposition();
        }
        else if (l_vnode == depend_vtree_node_id) {
            if (vtree_.is_left_descendant(depend_vtree_node_id, r_vnode)) {
                decomp_l = l_node.decomposition();                
                decomp_r = {{rhs, ZSDD_EMPTY}};
            }  else {
                decomp_l = l_node.decomposition();
                decomp_r = {{ZSDD_EMPTY, rhs}};
            }
        }
        else if (r_vnode == depend_vtree_node_id) {

            if (vtree_.is_left_descendant(depend_vtree_node_id, l_vnode)) {
                decomp_l = {{lhs, ZSDD_EMPTY}};
                decomp_r = r_node.decomposition();

            }  else {
                decomp_l = {{ZSDD_EMPTY, lhs}};
                decomp_r = r_node.decomposition();

                
            }
        }
        else { //depend node is a common ancestor
            if (vtree_.is_left_descendant(depend_vtree_node_id, l_vnode)) {
                decomp_l = {{lhs, ZSDD_EMPTY}};
                decomp_r = {{ZSDD_EMPTY, rhs}};

                
            } else {
                decomp_l = {{ZSDD_EMPTY, lhs}};
                decomp_r = {{rhs, ZSDD_EMPTY}};

            }
        }
    }
    
    std::vector<std::pair<addr_t, addr_t>> new_decomp_candidates;
    if (op == Operation::ORTHOGONAL_JOIN) {
        for (const auto l_elem : decomp_l) {
            for (const auto r_elem : decomp_r) {
                addr_t new_p = zsdd_apply(Operation::ORTHOGONAL_JOIN, l_elem.first, r_elem.first);
                if (new_p == ZSDD_NULL || new_p == ZSDD_FALSE) continue;
                
                addr_t new_s = zsdd_apply(op, l_elem.second, r_elem.second);
                if (new_s == ZSDD_NULL || new_s == ZSDD_FALSE) { 
                    continue;
                }
                new_decomp_candidates.emplace_back(new_p, new_s);
            }
        }
    }
    else {
        for (const auto l_elem : decomp_l) {
            for (const auto r_elem : decomp_r) {
                addr_t new_p = zsdd_apply(Operation::INTERSECTION,  l_elem.first, r_elem.first);
                if (new_p == ZSDD_NULL || new_p == ZSDD_FALSE) continue;
                
                addr_t new_s = zsdd_apply(op, l_elem.second, r_elem.second);
                if (new_s == ZSDD_NULL || new_s == ZSDD_FALSE) { 
                    continue;
                }
                new_decomp_candidates.emplace_back(new_p, new_s);
            }
        }
    }

    if (op == Operation::UNION || op == Operation::DIFFERENCE) { // op for implicit decomposition (rhs)
        addr_t r_primes_union = calc_primes_union(decomp_r);
        for (const auto l_elem : decomp_l) {
            addr_t new_p = zsdd_apply(Operation::DIFFERENCE, l_elem.first, r_primes_union);
            if (new_p == ZSDD_NULL || new_p == ZSDD_FALSE) continue;
            addr_t new_s = zsdd_apply(op, l_elem.second, ZSDD_FALSE);
            if (new_s == ZSDD_NULL || new_s == ZSDD_FALSE) { 
                continue;
            }
            new_decomp_candidates.emplace_back(new_p, new_s);
        }
    }
    if (op == Operation::UNION) { // op for implicit decomposition (lhs);
        addr_t l_primes_union = calc_primes_union(decomp_l);
        for (const auto r_elem : decomp_r) {
            addr_t new_p = zsdd_apply(Operation::DIFFERENCE, r_elem.first, l_primes_union);
            if (new_p == ZSDD_NULL || new_p == ZSDD_FALSE) continue;
            addr_t new_s = zsdd_apply(op, ZSDD_FALSE, r_elem.second);
            if (new_s == ZSDD_NULL || new_s == ZSDD_FALSE ) { 
                continue;
            }
            new_decomp_candidates.emplace_back(new_p, new_s);
        }
    }
    if (new_decomp_candidates.empty()) {
        cache_table_.write_cache(op, lhs, rhs, ZSDD_FALSE);
        return ZSDD_FALSE;
    }

    // compression

    auto new_decomposition = compress_candidates(new_decomp_candidates);

    // zero suppression
    if (new_decomposition.size() == 1) {
        // zero-suppression rule
        ZsddElement e = new_decomposition[0];
        if (e.first == ZSDD_EMPTY) {
            cache_table_.write_cache(op, lhs, rhs, e.second);
            return e.second;
        } 
        if (e.second == ZSDD_EMPTY) {
            cache_table_.write_cache(op, lhs, rhs, e.first);
            return e.first;
        }
    }
    addr_t result_node = make_zsdd_decomposition(std::move(new_decomposition),
                                                 depend_vtree_node_id);
    cache_table_.write_cache(op, lhs, rhs, result_node);
    return result_node;
}


std::vector<ZsddElement> ZsddManager::compress_candidates(const std::vector<std::pair<addr_t, addr_t>>&
                                                          new_decomp_candidates) {
    std::unordered_map<addr_t, std::vector<addr_t>> same_sub_groups;
    for (auto p : new_decomp_candidates) {
        if (same_sub_groups.find(p.second) == same_sub_groups.end()) {
            same_sub_groups.emplace(p.second, std::vector<addr_t>{p.first});
        } else {
            same_sub_groups.at(p.second).push_back(p.first);
        }
    }
    std::vector<ZsddElement> new_decomposition;

    for (auto& p : same_sub_groups) {
        auto same_sub_group = p.second;

        addr_t combined = ZSDD_FALSE;
        for (auto i : same_sub_group) {
            addr_t combined_prev = combined;
            combined = zsdd_apply(Operation::UNION, combined_prev, i);
        }
        new_decomposition.emplace_back(combined, p.first);
    }
    return new_decomposition;
}


unsigned long long ZsddManager::count_solution(const addr_t zsdd) const {
    std::unordered_map<addr_t, unsigned long long> cache;
    return count_solution_inner(zsdd, cache);
}


unsigned long long ZsddManager::count_solution_inner(const addr_t zsdd, 
                                                     std::unordered_map<addr_t, 
                                                     unsigned long long>& cache) const {
    if (zsdd == ZSDD_EMPTY) return 1LLU;
    if (zsdd == ZSDD_FALSE) return 0LLU;

    auto r = cache.find(zsdd);
    if (r != cache.end()) {
        return r->second;
    }
    
    const ZsddNode& n = get_zsddnode_at(zsdd);
    if (n.type() == NodeType::LIT) {
        if (n.literal() < 0) {
            cache.emplace(zsdd, 2LLU);
            return 2LLU;
        } else {
            cache.emplace(zsdd, 1LLU);
            return 1LLU;
        }
    }
    else { // n.type() == NodeType::DECOMP
        unsigned long long c = 0LLU;
        for (const auto e : n.decomposition()) {
            auto p_c = count_solution_inner(e.first, cache);
            auto s_c = count_solution_inner(e.second, cache);
            c += p_c * s_c;
        }
        cache.emplace(zsdd, c);
        return c;
    }
}


std::vector<std::vector<int>> ZsddManager::calc_setfamily(const addr_t zsdd) const {
    std::unordered_map<addr_t, std::vector<std::vector<int>>> cache;
    return calc_setfamily_inner(zsdd, cache);
}

std::vector<std::vector<int>> 
ZsddManager::calc_setfamily_inner(const addr_t zsdd, 
                                  std::unordered_map<addr_t, std::vector<std::vector<int>>>& cache) const {
    if (zsdd == ZSDD_EMPTY) return std::vector<std::vector<int>>(1, std::vector<int>());
    if (zsdd == ZSDD_FALSE) return std::vector<std::vector<int>>();

    auto r = cache.find(zsdd);
    if (r != cache.end()) {
        return r->second;
    }

    const ZsddNode& n = get_zsddnode_at(zsdd);
    std::vector<std::vector<int>> v;
    if (n.type() == NodeType::LIT) {
        if (n.literal() < 0) {
            v.push_back(std::vector<int>());
            v.push_back({-n.literal()});
        } else {
            v.push_back({n.literal()});
        }
    }
    else { // n.type() == NodeType::DECOMP
        for (const auto e : n.decomposition()) {
            const auto& p_v = calc_setfamily_inner(e.first, cache);
            const auto& s_v = calc_setfamily_inner(e.second, cache);

            for (const auto& x : p_v) {
                for (const auto& y : s_v) {
                    auto z = x;
                    z.insert(z.end(), y.begin(), y.end());
                    v.emplace_back(move(z));
                }
            }
        }
    }
    cache.emplace(zsdd, v);
    return v;
    
}


addr_t ZsddManager::make_zsdd_powerset_inner(const int vtree_node) {

    const VTreeNode& v = vtree_.get_node(vtree_node);
    if (v.is_leaf()) {
        return make_zsdd_literal_inner(-v.var());
    }
    else {
        addr_t c = cache_table_.read_cache(Operation::POWER_SET, vtree_node, vtree_node);
        if (c != ZSDD_NULL) {
            return c;
        }
        addr_t new_p = make_zsdd_powerset_inner(v.left_child());
        addr_t new_s = make_zsdd_powerset_inner(v.right_child());
        addr_t pset = make_zsdd_decomposition({{new_p, new_s}}, vtree_node);
        cache_table_.write_cache(Operation::POWER_SET, vtree_node, vtree_node, pset);
        return pset;
    }
}


unsigned long long int ZsddManager::size(const addr_t zsdd) const {
    if (zsdd < 0) return 0;
    
    std::unordered_set<addr_t> nodes;
    std::stack<addr_t> unexpanded;
    unexpanded.push(zsdd);
    while (!unexpanded.empty()) {
        addr_t e = unexpanded.top();
        unexpanded.pop();

        nodes.insert(e);
        if (e < 0) {
            continue;
        }                
        const ZsddNode& n = zsdd_node_table_.get_node_at(e);
        if (n.type() == NodeType::DECOMP) {
            for (const ZsddElement e : n.decomposition()) {
                if (nodes.find(e.first) == nodes.end()) {
                    nodes.insert(e.first);
                    unexpanded.push(e.first);
                }
                if (nodes.find(e.second) == nodes.end()) {
                    nodes.insert(e.second);
                    unexpanded.push(e.second);
                }
            }
        }
    }
    unsigned long long int size = 0LLU;
    for (const auto i : nodes) {
        if (i < 0) continue;
        const ZsddNode& n = get_zsddnode_at(i);
        if (n.type() == NodeType::DECOMP) {
            size += n.decomposition().size();
        }
    }
    return size;
}


Zsdd ZsddManager::zsdd_to_explicit_form(const Zsdd& zsdd) {
    addr_t z = zsdd_to_explicit_form_inner(zsdd.addr());
    return Zsdd(z, *this);
}


addr_t ZsddManager::zsdd_to_explicit_form_inner(const addr_t zsdd) {
    if (zsdd < 0) return zsdd;
    ZsddNode& n = get_zsddnode_at(zsdd);
    if (n.type() == NodeType::LIT) return zsdd;

    {
        addr_t c = cache_table_.read_cache(Operation::EXPLICIT_FORM, zsdd, zsdd);
        if (c != ZSDD_NULL) {
            return c;
        }
    }
    const int v_node = n.vtree_node_id();
    addr_t diff_p = make_zsdd_powerset_inner(vtree_.get_node(v_node).left_child());
    std::vector<ZsddElement> new_decomposition;
    for (const auto e : n.decomposition()) {
        diff_p = zsdd_apply(Operation::DIFFERENCE, diff_p, e.first);
        addr_t new_p = zsdd_to_explicit_form_inner(e.first);
        addr_t new_s = zsdd_to_explicit_form_inner(e.second);
        if (new_p == ZSDD_FALSE) continue;
        new_decomposition.emplace_back(new_p, new_s);
    }
    diff_p = zsdd_to_explicit_form_inner(diff_p);
    if (diff_p != ZSDD_FALSE) {
        new_decomposition.emplace_back(diff_p, ZSDD_FALSE);
    }
    addr_t res = make_zsdd_decomposition(std::move(new_decomposition), v_node);
    cache_table_.write_cache(Operation::EXPLICIT_FORM, zsdd, zsdd, res);
    return res;
}


void  ZsddManager::export_zsdd_txt_inner(const addr_t zsdd, std::ostream& os, 
                                         std::unordered_set<addr_t>& found, 
                                         const size_t empty_id, 
                                         const size_t false_id) const {
    const ZsddNode& node = get_zsddnode_at(zsdd);
    assert(node.type() != NodeType::UNUSED);
    if (node.type() == NodeType::LIT) {
        os << "L " << zsdd << " " << node.vtree_node_id() 
           << " " << node.literal() << std::endl;
        return;
    }
    
    for (const auto e : node.decomposition()) {
        if (e.first >= 0 && (found.find(e.first) == found.end()))  {
            found.insert(e.first);
            export_zsdd_txt_inner(e.first, os, found, empty_id, false_id);
        }
        if (e.second >= 0 && (found.find(e.second) == found.end()))  {
            found.insert(e.second);
            export_zsdd_txt_inner(e.second, os, found, empty_id, false_id);
        }
    }
    os << "D " << zsdd << " " << node.vtree_node_id() << " "
       << node.decomposition().size();
    for (const auto e : node.decomposition()) {
        auto func = [empty_id, false_id](addr_t i) -> addr_t {
            if (i == -1) return empty_id;
            if (i == -2) return false_id;
            return i;
        };
        addr_t p = func(e.first);
        addr_t s = func(e.second);
        os << " " << p << " " << s;
    }
    os << std::endl;
}

void  ZsddManager::export_zsdd_txt(const addr_t zsdd, std::ostream& os) const {

    // print header
    os << 
        "c ids of zsdd nodes start at 0\n"
        "c zsdd nodes appear bottom-up, children before parents\n"
        "c The empty constant node corresponds to id -1\n"
        "c The false constant node corresponds to id -2\n"
        "c\n"
        "c file syntax:\n"
        "c zsdd count-of-zsdd-nodes\n"
        "c F id-of-false-sdd-node\n"
        "c E id-of-empty-sdd-node\n"
        "c L id-of-literal-sdd-node id-of-vtree literal\n"
        "c D id-of-decomposition-sdd-node id-of-vtree number-of-elements {id-of-prime id-of-sub}*\n"
        "c\n";
    if (zsdd == -1) {
        os << "zsdd \nE 0" << std::endl;
        return;
    } else if (zsdd == -2) {
        os << "zsdd \nF 0" << std::endl;
        return;
    } 
    // zsdd >= 0
    os << "zsdd " << size(zsdd) << std::endl;
    auto empty_id = zsdd_node_table_.node_array_size();
    auto false_id = empty_id + 1;
    os << "E " << empty_id << std::endl;
    os << "F " << false_id << std::endl;

    std::unordered_set<addr_t> found;
    export_zsdd_txt_inner(zsdd, os, found, empty_id, false_id);
}


void  ZsddManager::export_zsdd_dot(const addr_t zsdd, std::ostream& os) const {
    

    const std::string SYMBOL_EMPTY = "&#949;";
    const std::string SYMBOL_FALSE = "&#8869;";
    auto  lit2symb =  [](const int literal) -> std::string {
        std::ostringstream oss;
        if (literal < 0) {
            oss << "&#177;";
        }
        if (abs(literal) <= 'Z' - 'A' + 1) {
            oss << static_cast<char>(abs(literal) + 'A' - 1);
        } else {
            oss << abs(literal);
        }
        return oss.str();
    };

    if (zsdd < 0 || get_zsddnode_at(zsdd).type() == NodeType::LIT) {
        std::string symbol;
        if (zsdd == -1) {
            symbol = SYMBOL_EMPTY;
        } else if (zsdd == -2) {
            symbol = SYMBOL_FALSE;
        } else {
            int lit =  get_zsddnode_at(zsdd).literal();
            std::ostringstream oss;
            symbol = lit2symb(lit);
        }
        os << "digraph zsdd {\noverlap=false\n"
           << "n1 [label= \"" <<  symbol << "\",\n"
           << "shape=record,\n" 
           << "fontsize=20,\n"
           << "fontname=\"Times-Italic\",\n"
           << "fillcolor=white,\n"
           << "style=filled,\n"
           << "fixedsize=true,\n"
           << "height=.30,\n"
           << "width=.45];\n"
           << "}\n";
        
        return;
    }

    std::unordered_map<int, std::vector<addr_t> > same_level_nodes;
    {
        std::unordered_set<addr_t> visited;
        std::stack<addr_t> stk;
        stk.push(zsdd);
        visited.insert(zsdd);
        while (!stk.empty()) {
            auto addr = stk.top();
            stk.pop();
            auto& node = get_zsddnode_at(addr);
            if (node.type() == NodeType::DECOMP) {
                if (same_level_nodes.find(node.vtree_node_id()) == same_level_nodes.end()) {
                    same_level_nodes[node.vtree_node_id()] = std::vector<addr_t>();
                }
                same_level_nodes[node.vtree_node_id()].push_back(addr);
                
                for (const auto e : node.decomposition()) {
                    if (e.first >= 0 && visited.find(e.first) == visited.end())  {
                        stk.push(e.first);
                        visited.insert(e.first);
                    }
                    if (e.second >= 0 && visited.find(e.second) == visited.end())  {
                        stk.push(e.second);
                        visited.insert(e.second);
                    }
                }
            }
        }
    }

    
    
    os << "digraph zsdd {\noverlap=false\n";
    for (auto& pair : same_level_nodes) {
        os << "{rank=same;";
        for (auto& id : pair.second) {
            os << " n" << id;
        }
        os << "}\n";
    }

    std::unordered_set<addr_t> visited;
    std::stack<addr_t> stk;
    stk.push(zsdd);
    visited.insert(zsdd);
    while (!stk.empty()) {
        auto addr = stk.top();
        stk.pop();
        auto& node = get_zsddnode_at(addr);
        if (node.type() == NodeType::DECOMP) {
            os << "n" << addr
               << "[label= \"" << node.vtree_node_id() 
               << "\",style=filled,fillcolor=gray95,shape=circle,height=.25,width=.25]; \n";
            
            for (size_t i = 0; i < node.decomposition().size(); i++) {
                const auto& elem = node.decomposition().at(i);
                
                std::string  p_s;
                std::string  s_s;
                
                if (elem.first == -1) {
                    p_s = SYMBOL_EMPTY;
                } else if (elem.first == -2) {
                    p_s =  SYMBOL_FALSE;
                } else {
                    const auto& prime = get_zsddnode_at(elem.first);
                    if (prime.type() == NodeType::LIT) {
                        p_s = lit2symb(prime.literal());
                    } else {
                        if (visited.find(elem.first) == visited.end()) {
                            visited.insert(elem.first);
                            stk.push(elem.first);
                        }
                    }
                }
                if (elem.second == -1) {
                    s_s = SYMBOL_EMPTY;
                } else if (elem.second == -2) {
                    s_s =  SYMBOL_FALSE;
                } else {
                    const auto& sub = get_zsddnode_at(elem.second);
                    if (sub.type() == NodeType::LIT) {
                        s_s = lit2symb(sub.literal());
                    } else {
                        if (visited.find(elem.second) == visited.end()) {
                            visited.insert(elem.second);
                            stk.push(elem.second);
                        }
                    }
                }
                std::ostringstream nid_oss;
                nid_oss << "n" << addr << "e" << i;
                auto nid = nid_oss.str();
                os << nid
                   << " [label= \"<L>" <<  p_s 
                   << "|<R>" << s_s << "\",\n"
                   << "shape=record,\n"
                    "fontsize=20,\n"
                    "fontname=\"Times-Italic\",\n"
                    "fillcolor=white,\n"
                    "style=filled,\n"
                    "fixedsize=true,\n"
                    "height=.30,\n"
                    "width=.65];\n";
                os << "n" << addr << "->" << nid << " [arraysize=.50];";
                if (p_s == "") {
                    os << nid <<  ":L:c->n" 
                       << elem.first <<  "[arrowsize=.50,tailclip=false,arrowtail=dot,dir=both];\n";
                }
                if (s_s == "") {
                    os << nid <<  ":R:c->n" 
                       << elem.second <<  "[arrowsize=.50,tailclip=false,arrowtail=dot,dir=both];\n";
                }
            }
        }
 
    }
    os << "}\n";   
}
} // namespace zsdd

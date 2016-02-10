#include "zsdd_vtree.h"
#include <algorithm>
#include <stack>
#include <fstream>
#include <sstream>
#include <assert.h>

namespace zsdd {

void VTree::setup_literal_vid_map() {
    literal_vid_map_.clear();
    for (int i = 0; i < (int)tree_nodes_.size(); i++) {
        const VTreeNode& n = tree_nodes_[i];
        if (n.is_leaf()) {
            literal_vid_map_.emplace(n.var(), i);
        }
    }
}


void VTree::setup_node_depth() {
    assert(node_depth_.size() == tree_nodes_.size());
    // find the root 
    int root_idx = -1;
    for (int i = 0; i < (int)tree_nodes_.size(); i++) {
        const VTreeNode& n = tree_nodes_[i];
        if (n.parent() < 0) {
            root_idx = i;
            break;
        }
    }
    assert(root_idx >= 0 && root_idx < (int)tree_nodes_.size());
    std::stack<std::pair<int, int>> unexpanded;
    unexpanded.push(std::make_pair(root_idx, 0));
    while (!unexpanded.empty()) {
        auto p = unexpanded.top();
        unexpanded.pop();
        node_depth_[p.first] = p.second;
        const VTreeNode& n = tree_nodes_[p.first];
        if (!n.is_leaf()) {
            unexpanded.push(std::make_pair(n.left_child(), p.second+1));
            unexpanded.push(std::make_pair(n.right_child(), p.second+1));
        }
    }
}


int VTree::get_depend_node(const int lhs_id, const int rhs_id) const {
    assert(lhs_id >= 0 && lhs_id < (int)tree_nodes_.size() &&
           rhs_id >= 0 && rhs_id < (int)tree_nodes_.size());
    if (lhs_id == rhs_id) return lhs_id;
    auto l_d = node_depth_[lhs_id];
    auto r_d = node_depth_[rhs_id];
    int l = lhs_id;
    int r = rhs_id;

    if (l_d < r_d) {
        r = get_ancestor_at_depth(r, l_d);
    } else if (r_d < l_d) {
        l = get_ancestor_at_depth(l, r_d);        
    }
    if (l == r) return l;
    while (l != r) {
        l = tree_nodes_[l].parent();
        r = tree_nodes_[r].parent();

        assert(l >= 0 && r >= 0);
    }
    return l;
}


bool VTree::is_left_descendant(const int parent, const int child) const {
    if (tree_nodes_[parent].is_leaf()) return false;
    auto l = tree_nodes_[parent].left_child();
    return l == get_depend_node(l, child);
}


bool VTree::is_right_descendant(const int parent, const int child) const {
    if (tree_nodes_[parent].is_leaf()) return false;
    auto r = tree_nodes_[parent].right_child();

    return r == get_depend_node(r, child);
}


int VTree::get_ancestor_at_depth(const int node, const int depth) const {
    const int d = node_depth_[node];
    assert (d > depth);
    int n = node;
    for (int i = 0; i < d - depth; i++) {
        n = tree_nodes_[n].parent();
    }
    assert(n >= 0);
    return n;
}


int VTree::find_literal_node_id(const int literal) const {
    auto res = literal_vid_map_.find(labs(literal));
    if (res == literal_vid_map_.end()) {
        std::cerr << "[error] can't find literal " << literal << std::endl;
        exit(1);
    }
    return res->second;
}


VTree VTree::import_from_sdd_vtree_file(const std::string& file_name) {
    std::ifstream ifs(file_name);

    std::string line;
    bool on_reading_header = true;

    std::unordered_map<int, std::tuple<int, int, int>> elems;
    std::vector<int> parents;

    while (getline(ifs, line)) {
        if (line[0] == 'c') continue;
        if (on_reading_header) {
            on_reading_header = false;
            std::istringstream is(line);
            std::string type;
            int num_nodes;    
            is >> type;
            is >> num_nodes;
            assert (type== "vtree");
            parents.assign(num_nodes, -1);
        } else {
            std::istringstream is(line);
            char type;

            is >> type;
            if (type == 'L') {
                int nid;
                int var;                
                is >> nid;
                is >> var;
                std::tuple<int, int, int> elem(nid, var, -1);
                elems.emplace(nid, move(elem));
            } else if (type == 'I') {
                int nid;
                int left;  
                int right;
                is >> nid;
                is >> left;
                is >> right;
                parents[left] = nid;
                parents[right] = nid;
                std::tuple<int, int, int> elem(nid, left, right);
                elems.emplace(nid, move(elem));
            }
        }
    }
    std::vector<VTreeNode> vtree_nodes;

    for (int i = 0; i < (int)parents.size(); i++) {
        const auto& t = elems.at(i);
        if (std::get<2>(t) == -1) {
            // is leaf node
            vtree_nodes.emplace_back(std::get<1>(t), parents[i]);
        } else {
            // is branch node
            vtree_nodes.emplace_back(std::get<1>(t), std::get<2>(t), parents[i]);
        }
    }
    return VTree(vtree_nodes);
}

VTree VTree::construct_right_linear_vtree(const unsigned int num_vars) {
    std::vector<VTreeNode> vtree_nodes;
    int parent_id = -1;
    for (unsigned int i = 0; i < num_vars-1; i++) {
        vtree_nodes.emplace_back(2*i + 1, 2 * i + 2, parent_id); // branch node
        parent_id = 2 * i;
        vtree_nodes.emplace_back(i + 1 , parent_id); // leaf node
    }
    vtree_nodes.emplace_back(num_vars, parent_id);
    return VTree(vtree_nodes);
}



} // namespace zsdd

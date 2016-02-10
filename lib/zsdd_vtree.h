#ifndef ZSDD_VTREE_H_
#define ZSDD_VTREE_H_

#include <assert.h>
#include <vector>
#include <iostream>
#include "zsdd_common.h"
#include <unordered_map>


namespace zsdd {

class VTreeNode {
public:
    VTreeNode(const int left_child, const int right_child, 
              const int parent) :
        left_child_(left_child), right_child_(right_child),
        parent_(parent) {}

    VTreeNode(const VTreeNode& obj) :
        left_child_(obj.left_child_), right_child_(obj.right_child_),
        parent_(obj.parent_) {}


    VTreeNode(const int var, const int parent) :
        left_child_(-var), right_child_(0), parent_(parent) {assert(var>0);}
    bool is_leaf() const { return left_child_ < 0;}
    int left_child() const { return left_child_;}
    int right_child() const { return right_child_;}
    int parent() const { return parent_;}
    int var() const { return -left_child_; }


    
private:
    const int left_child_;
    const int right_child_;
    const int parent_;
    friend std::ostream& operator<<(std::ostream& os, const VTreeNode& n) {
        os << "(" << n.left_child_ << ", " << n.right_child_ << ", " << n.parent_ << ")";
        return os;
    }
};

class VTree {
public:
    VTree(const std::vector<VTreeNode>& tree_nodes) :
        tree_nodes_(tree_nodes), node_depth_(tree_nodes.size(),0) {
        setup_literal_vid_map();
        setup_node_depth();
    }

    VTree(const VTree& obj) :
        tree_nodes_(obj.tree_nodes_), 
        literal_vid_map_(obj.literal_vid_map_), 
        node_depth_(obj.node_depth_) {}

    const VTreeNode& get_node(const int i) const {
        return tree_nodes_[i];
    }
    int get_depend_node(const int lhs_id, const int rhs_id) const;
    bool is_left_descendant(const int parent, const int child) const;
    bool is_right_descendant(const int parent, const int child) const;
    int find_literal_node_id(const int literal) const;
    int get_ancestor_at_depth(const int node, const int depth) const;

    static VTree import_from_sdd_vtree_file(const std::string& file_name);
    static VTree construct_right_linear_vtree(const unsigned int num_vars);

private:
    const std::vector<VTreeNode> tree_nodes_;
    std::unordered_map<int, int> literal_vid_map_;
    std::vector<int> node_depth_;
    void setup_literal_vid_map();
    void setup_node_depth();

};

} // namespace zsdd

#endif // ZSDD_VTREE_H_

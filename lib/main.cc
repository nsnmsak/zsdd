#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include "zsdd.h"
using namespace std;
using namespace zsdd;

std::vector<int> split(const std::string& input, char delimiter)
{
    std::istringstream stream(input);

    std::string field;
    std::vector<int> result;
    while (std::getline(stream, field, delimiter)) {
        result.push_back(stoi(field));
    }
    return result;
}



Zsdd make_power_set(const unordered_set<int>& ids, ZsddManager& mgr) {
    vector<int> ids_sort(ids.begin(), ids.end());
    sort(ids_sort.begin(), ids_sort.end());

    Zsdd z = mgr.make_zsdd_baseset();
    for (auto i : ids_sort) {
        Zsdd e = mgr.make_zsdd_literal(-i);
        z = mgr.zsdd_orthogonal_join(z, e);
    }
    return z;
}

Zsdd make_zsdd_cnf_clause(const vector<int>& clause, 
                          unordered_set<int>& all_variables, 
                          ZsddManager& mgr) {
    unordered_set<int> diff = all_variables;
    unordered_set<int> cls_vars;
    for (auto l : clause) {
        diff.erase(abs(l));
        cls_vars.insert(abs(l));
    }
    Zsdd diff_set = make_power_set(diff, mgr);
    Zsdd clause_set = make_power_set(cls_vars, mgr);
    Zsdd unsat_set = clause_set;
    for (auto l : clause) {
        if (l > 0) {
            unsat_set = mgr.zsdd_filter_not_contain(unsat_set, l);
        } else {
            unsat_set = mgr.zsdd_filter_contain(unsat_set, -l);
        }
    }
    clause_set = mgr.zsdd_difference(clause_set, unsat_set);

    return mgr.zsdd_orthogonal_join(clause_set, diff_set);
}

Zsdd make_zsdd_dnf_term(const vector<int>& term, 
                          unordered_set<int>& all_variables, 
                          ZsddManager& mgr) {
    unordered_set<int> diff = all_variables;

    for (auto l : term) {
        diff.erase(abs(l));
    }
    Zsdd diff_set = make_power_set(diff, mgr);
    Zsdd term_set = mgr.make_zsdd_baseset();
    for (auto l : term) {
        if (l > 0) {
            term_set = mgr.zsdd_change(term_set, l);
        }
    }
    return mgr.zsdd_orthogonal_join(term_set, diff_set);
}


Zsdd compile_dnf(const vector<vector<int>>& dnf, const int num_variables, ZsddManager& mgr) {
    unordered_set<int> all_variables;
    for (int i = 1; i <= num_variables; i++) {
        all_variables.insert(i);
    }
    vector<Zsdd> term_zsdds;
    for (auto term : dnf) {
        term_zsdds.push_back(make_zsdd_dnf_term(term, all_variables, mgr));
    }
    mgr.gc();
    
    while (term_zsdds.size() > 1) {
        vector<Zsdd> tmp_zsdds;
        for (int i = 0; i < (int)(term_zsdds.size() + 1) / 2; i++) {
            if (2 * i + 1 >= (int)term_zsdds.size()) {
                tmp_zsdds.push_back(term_zsdds[2*i]);
            }
            else {
                tmp_zsdds.push_back(mgr.zsdd_union(term_zsdds[2*i], term_zsdds[2*i+1]));
            }
        }
        term_zsdds = tmp_zsdds;
        mgr.gc();
    }
    return term_zsdds[0];
}

Zsdd compile_cnf(const vector<vector<int>>& cnf, const int num_variables, ZsddManager& mgr) {
    unordered_set<int> all_variables;
    for (int i = 1; i <= num_variables; i++) {
        all_variables.insert(i);
    }
    vector<Zsdd> clause_zsdds;
    for (auto& clause : cnf) {
        clause_zsdds.push_back(make_zsdd_cnf_clause(clause, all_variables, mgr));
    }
    while (clause_zsdds.size() > 1) {
        vector<Zsdd> tmp_zsdds;
        for (int i = 0; i < (int)(clause_zsdds.size() + 1) / 2; i++) {
            if (2 * i + 1 >= (int)clause_zsdds.size()) {
                tmp_zsdds.push_back(clause_zsdds[2*i]);
            }
            else {
                tmp_zsdds.push_back(mgr.zsdd_intersection(clause_zsdds[2*i], clause_zsdds[2*i+1]));
            }
        }
        clause_zsdds = tmp_zsdds;
        mgr.gc();
    }
    return clause_zsdds[0];
}

vector<vector<int>> read_fnf(const string& file_name, int* num_variables) {
    // read dimacs form cnf/dnf format
    vector<vector<int>> fnf;
    ifstream ifs(file_name);
    if (ifs.fail()) {
        cerr << "can't read " << file_name << endl;
        exit(1);
    }
    string line;

    bool on_reading_header = true;
    
    while (getline(ifs, line)) {
        if (line[0] == 'c') continue;
        if (on_reading_header) {
            assert(line[0] == 'p');
            on_reading_header = false;
            istringstream is(line);
            string p;
            string form;
            is >> p;
            is >> form;
            is >> *num_variables;
        } 
        else {
            istringstream is(line);
            vector<int> clause;
            while (!is.eof()) {
                int e;
                is >> e;
                clause.push_back(e);
            }
            assert(*(clause.rbegin()) == 0);
            clause.pop_back();
            fnf.push_back(move(clause));
        }
    }

    return fnf;
}


vector<vector<int>> read_setset(const string& file_name) {
    ifstream ifs(file_name);
    if (ifs.fail()) {
        cerr << "can't read " << file_name << endl;
        exit(1);
    }
    string line;
    vector<vector<int>> setset;
    while (getline(ifs, line)) {
        setset.push_back(split(line, ' '));
    }
    return setset;
}


void show_help_and_exit() {
    cout << "zsdd: Zero-suppressed Sentential Decision Diagrams\n"
         << "zsdd [-c .] [-d .] [-v .] [-R .] [-S .]  [-h]\n"
         << "    -c FILE        set input CNF file\n"
         << "    -d FILE        set input DNF file\n"
         << "    -v FILE        set input VTREE file (default is a right-linear vtree)\n"
         << "    -e             use zsdd without implicit partitioning"
         << "    -R FILE        set output ZSDD file\n"
         << "    -S FILE        set output ZSDD (dot) file\n"
         << "    -h             show help message and exit\n";
    exit(1);
}



int main(int argc, char** argv)
{
    int opt;
    string vtree_file_name = "";
    string cnf_input_file_name = "";
    string dnf_input_file_name = "";
    string txt_output_file_name = "";
    string dot_output_file_name = "";
    bool use_explicit_representation = false;
    while ((opt = getopt(argc, argv, "v:c:d:e:R:S:h")) != -1) {
        switch (opt) {
        case 'v':
            vtree_file_name = optarg;
            break;
        case 'c':
            cnf_input_file_name = optarg;
            break;
        case 'd':
            dnf_input_file_name = optarg;
            break;
        case 'e':
            use_explicit_representation = true;
            break;
        case 'R':
            txt_output_file_name = optarg;
            break;
        case 'S':
            dot_output_file_name = optarg;
            break;
        case 'h':
            show_help_and_exit();
            break;
        }
    }
    if (cnf_input_file_name == "" && dnf_input_file_name == "") {
        show_help_and_exit();
    }

    VTree* vtree = nullptr;

    auto compiler = compile_dnf;
    if (cnf_input_file_name != "") {
        compiler = compile_cnf;
    }
    vector<vector<int>> fnf;
    int num_variables;
    if (cnf_input_file_name != "") {
        fnf = read_fnf(cnf_input_file_name, &num_variables);
        cerr << "reading cnf... vars=" << num_variables << " clauses=" << fnf.size() << endl;
    } else {
        fnf = read_fnf(dnf_input_file_name, &num_variables);
        cerr << "reading dnf... vars=" << num_variables << " terms=" << fnf.size() << endl;
    }

    if (vtree_file_name != "") {
        vtree = new VTree(VTree::import_from_sdd_vtree_file(vtree_file_name));
        cerr << "loading vtree..." << endl;
    } else {
        vtree = new VTree( VTree::construct_right_linear_vtree(num_variables));
        cerr << "creating vtree (right-linear)..." << endl;
    }
    ZsddManager mgr(*vtree,  1U<<24);

    cerr << "compiling..." << endl;
    auto compile_start = chrono::system_clock::now();
    Zsdd zsdd = compiler(fnf, num_variables, mgr);
    if (use_explicit_representation) {
        zsdd = mgr.zsdd_to_explicit_form(zsdd);
    }
    auto compile_end = chrono::system_clock::now();
    cerr << "compilation time: "
         << chrono::duration_cast<chrono::milliseconds>(compile_end - compile_start).count() 
         << " msec" << endl;

    cerr << "zsdd node count: " << zsdd.size() << endl;
    cerr << "zsdd model count: " << zsdd.count_solution() << endl;

    if (txt_output_file_name != "") {
        cerr << "output zsdd..." << endl;
        ofstream ofs(txt_output_file_name);
        zsdd.export_txt(ofs);
        ofs.close();
    }

    if (dot_output_file_name != "") {
        cerr << "output zsdd (dot)..." << endl;
        ofstream ofs(dot_output_file_name);
        zsdd.export_dot(ofs);
        ofs.close();
    }
    
    return 0;
}

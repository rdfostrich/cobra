#include <iostream>
#include <kchashdb.h>
#include <rdf/RDFParserNtriples.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/controller/controller.h"
#include "snapshot/combined_triple_iterator.h"
#include "simpleprogresslistener.h"

using namespace std;
/*
 * COPIES PATCH TREE
 */
int main(int argc, char** argv) {
    if(argc != 2){
        cerr << "ERROR: Copy command must be invoked as 'patchtree_id'" << endl;
        return 1;
    }

    int patch_tree_id = std::atoi(argv[1]);

    // Load the store
    Controller controller("./", TreeDB::TCOMPRESS);
    controller.copy_patch_tree_files("", patch_tree_id);
    return 0;
}
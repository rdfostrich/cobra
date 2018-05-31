#include <iostream>
#include <kchashdb.h>
#include <rdf/RDFParserNtriples.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/controller/controller.h"
#include "snapshot/combined_triple_iterator.h"
#include "simpleprogresslistener.h"

#define BASEURI "<http://example.org>"

using namespace std;

int main(int argc, char** argv) {

    if (argc < 3) {
        cerr << "ERROR: Reverse command must be invoked as '[-v] [patch_id additions.nt deletions.nt]*'" << endl;
        return 1;
    }

    bool verbose = std::string(argv[1]) == "-v";
    ProgressListener* progressListener = verbose ? new SimpleProgressListener() : NULL;

    // Load the store
    Controller* controller = new Controller("./", TreeDB::TCOMPRESS);

    int patch_tree_id = std::atoi(argv[verbose+1]);

    controller->replace_patch_tree("", controller->get_patch_tree_id(patch_tree_id));

    controller->get_patch_tree_manager()->construct_next_patch_tree(patch_tree_id, controller->get_snapshot_manager()->get_dictionary_manager(patch_tree_id + 1));
    delete controller;

    for(int patch=verbose+1; patch < argc; patch+=3){
        Controller controller("./", TreeDB::TCOMPRESS);
        int snapshot_id = controller.get_corresponding_snapshot_id(patch_tree_id);
        controller.get_snapshot_manager()->load_snapshot(snapshot_id);
        DictionaryManager* dict = controller.get_snapshot_manager()->get_dictionary_manager(snapshot_id);

        int patch_id = std::atoi(argv[patch]);
        PatchElementIteratorCombined* it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));

        // read additions
        std::string file_1(argv[patch + 1]);
        ifstream f_1(file_1.c_str());
        if (!f_1.good()) {
            cerr << "Could not find a file at location: " << file_1 << endl;
            return 1;
        }
        f_1.close();
        IteratorTripleString *file_it = new RDFParserNtriples(file_1.c_str(), NTRIPLES);
        it_patch->appendIterator(new PatchElementIteratorTripleStrings(dict, file_it, true));

        // read deletions
        std::string file_2(argv[patch + 2]);
        ifstream f_2(file_2.c_str());
        if (!f_2.good()) {
            cerr << "Could not find a file at location: " << file_2 << endl;
            return 1;
        }
        f_2.close();
        file_it = new RDFParserNtriples(file_2.c_str(), NTRIPLES);
        it_patch->appendIterator(new PatchElementIteratorTripleStrings(dict, file_it, false));

        //insert patch
        controller.reverse_append(it_patch, patch_id, dict, false, progressListener);
        long long int added = it_patch->getPassed();
        delete it_patch;
        NOTIFYMSG(progressListener, ("\nInserted " + to_string(added) + " for version " + to_string(patch_id) + ".\n").c_str());
    }
    delete progressListener;
    return 0;
}

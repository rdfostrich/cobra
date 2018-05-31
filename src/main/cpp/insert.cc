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
    cerr << "Deprecated for now, insert with bear.cc" << endl;

    if (argc < 2) {
        cerr << "ERROR: Insert command must be invoked as '[-v] patch_id s|p [+|- cd.nt [file_2.nt [...]]]*' " << endl;
        return 1;
    }

    bool verbose = std::string(argv[1]) == "-v";
    ProgressListener* progressListener = verbose ? new SimpleProgressListener() : NULL;

    // Load the store
    Controller controller("./", TreeDB::TCOMPRESS);

    // Get parameters
    int patch_id = std::atoi(argv[1 + verbose]);
    bool snapshot;
    if (std::string(argv[2 + verbose]) == "p" || std::string(argv[2 + verbose]) == "s") {
        snapshot = std::string(argv[2 + verbose]) == "s";
    }
    else{
        cerr << "ERROR: Insert command must be invoked as '[-v] patch_id s|p [+|- cd.nt [file_2.nt [...]]]*' " << endl;
        return 1;
    }
    DictionaryManager* dict;

    // Initialize iterators
    CombinedTripleIterator* it_snapshot;
    PatchElementIteratorCombined* it_patch;
    if (snapshot) {
        it_snapshot = new CombinedTripleIterator();
    } else {
        int snapshot_id = controller.get_corresponding_snapshot_id(patch_id);
        controller.get_snapshot_manager()->load_snapshot(snapshot_id);
        dict = controller.get_snapshot_manager()->get_dictionary_manager(snapshot_id);
        it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    }

    // Append files to iterator
    bool additions = true;
    for (int file_id = 3 + verbose; file_id < argc; file_id++) {
        std::string file(argv[file_id]);
        if (file == "+" || file == "-") {
            additions = file == "+";
        } else {
            ifstream f(file.c_str());
            if (!f.good()) {
                cerr << "Could not find a file at location: " << file << endl;
                return 1;
            }
            IteratorTripleString *file_it = new RDFParserNtriples(file.c_str(), NTRIPLES);
            f.close();
            if (snapshot) {
                if (!additions) {
                    cerr << "Initial versions can not contain deletions" << endl;
                    return 1;
                }
                it_snapshot->appendIterator(file_it);
            } else {
                it_patch->appendIterator(new PatchElementIteratorTripleStrings(dict, file_it, additions));
            }
        }
    }

    long long int added;
    if (snapshot) {
        NOTIFYMSG(progressListener, "\nCreating snapshot...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT* hdt = controller.get_snapshot_manager()->create_snapshot(patch_id, it_snapshot, BASEURI, progressListener);
        std::cout.clear();
        added = hdt->getTriples()->getNumberOfElements();
        delete it_snapshot;
    } else {
        NOTIFYMSG(progressListener, "\nAppending patch...\n");
        int snapshot_id = controller.get_corresponding_snapshot_id(patch_id);
        controller.append(it_patch, patch_id, dict, false, progressListener);
        added = it_patch->getPassed();
        delete it_patch;
    }

    NOTIFYMSG(progressListener, ("\nInserted " + to_string(added) + " for version " + to_string(patch_id) + ".\n").c_str());
    delete progressListener;

    return 0;
}

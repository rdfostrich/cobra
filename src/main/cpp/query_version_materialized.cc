#include <iostream>
#include <kchashdb.h>
#include <unistd.h>
#include <cstring>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../main/cpp/controller/controller.h"

#define BASEURI "<http://example.org>"

using namespace std;
using namespace kyotocabinet;

int main(int argc, char** argv) {
    if (argc < 6 || argc > 7) {
        cerr << "ERROR: Query command must be invoked as 'base_path patch_id subject predicate object [offset]' " << endl;
        return 1;
    }

    // Load the store
    Controller controller(std::string(argv[1]), TreeDB::TCOMPRESS, true);

    // Get query parameters
    std::string s(argv[3]);
    std::string p(argv[4]);
    std::string o(argv[5]);
    if(std::strcmp(s.c_str(), "?") == 0) s = "";
    if(std::strcmp(p.c_str(), "?") == 0) p = "";
    if(std::strcmp(o.c_str(), "?") == 0) o = "";

    int patch_id = std::atoi(argv[2]);
    int offset = argc == 7 ? std::atoi(argv[6]) : 0;

    // Construct query
    DictionaryManager* dict = controller.get_dictionary_manager(patch_id);
    Triple triple_pattern(s, p, o, dict);

    std::pair<size_t, ResultEstimationType> count = controller.get_version_materialized_count(triple_pattern, patch_id, true);
    cerr << "Count: " << count.first << (count.second == EXACT ? "" : " (estimate)") << endl;

    TripleIterator* it = controller.get_version_materialized(triple_pattern, offset, patch_id);
    Triple triple(0, 0, 0);
    while (it->next(&triple)) {
        cout << triple.to_string(*dict) << endl;
    }
    delete it;

    return 0;
}

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
    if (argc < 5 || argc > 6) {
        cerr << "ERROR: Query command must be invoked as 'base_path subject predicate object [offset]' " << endl;
        return 1;
    }

    // Load the store
    Controller controller(std::string(argv[1]), TreeDB::TCOMPRESS, true);

    // Get query parameters
    std::string s(argv[2]);
    std::string p(argv[3]);
    std::string o(argv[4]);
    if(std::strcmp(s.c_str(), "?") == 0) s = "";
    if(std::strcmp(p.c_str(), "?") == 0) p = "";
    if(std::strcmp(o.c_str(), "?") == 0) o = "";

    int offset = argc == 6 ? std::atoi(argv[5]) : 0;

    // Construct query
    DictionaryManager* dict = controller.get_dictionary_manager(0);
    Triple triple_pattern(s, p, o, dict);

    std::pair<size_t, ResultEstimationType> count = controller.get_partial_version_count(triple_pattern, true);
    cerr << "Count: " << count.first << (count.second == EXACT ? "" : " (estimate)") << endl;

    TripleVersionsIterator* it = controller.get_partial_version(triple_pattern, offset);
    TripleVersions triple_versions;
    while (it->next(&triple_versions)) {
        std::stringstream vect;
        std::copy(triple_versions.get_versions()->begin(), triple_versions.get_versions()->end(), std::ostream_iterator<int>(vect, " "));
        cout << triple_versions.get_triple()->to_string(*dict) << " :: [ " << vect.str() << "]" << endl;
    }
    delete it;

    return 0;
}

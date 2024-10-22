#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "triple_store.h"
#include "patch_tree_addition_value.h"
#include "patch_tree_key_comparator.h"
#include "../simpleprogresslistener.h"

using namespace std;
using namespace kyotocabinet;

TripleStore::TripleStore(string base_file_name, DictionaryManager* dict, int8_t kc_opts, bool readonly) : dict(dict) {
    // Construct trees
    index_spo_deletions = new TreeDB();
    index_pos_deletions = new TreeDB();
    index_osp_deletions = new TreeDB();
    index_spo_additions = new TreeDB();
    index_pos_additions = new TreeDB();
    index_osp_additions = new TreeDB();
    count_additions = new HashDB();
    temp_count_additions = readonly ? NULL : new HashDB();

    // Set the triple comparators
    index_spo_deletions->tune_comparator(spo_comparator = new PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    index_pos_deletions->tune_comparator(pos_comparator = new PatchTreeKeyComparator(comp_p, comp_o, comp_s, dict));
    index_osp_deletions->tune_comparator(osp_comparator = new PatchTreeKeyComparator(comp_o, comp_s, comp_p, dict));
    index_spo_additions->tune_comparator(spo_comparator);
    index_pos_additions->tune_comparator(pos_comparator);
    index_osp_additions->tune_comparator(osp_comparator);
    element_comparator = new PatchElementComparator(spo_comparator);

    index_spo_deletions->tune_options(kc_opts);
    index_pos_deletions->tune_options(kc_opts);
    index_osp_deletions->tune_options(kc_opts);
    index_spo_additions->tune_options(kc_opts);
    index_pos_additions->tune_options(kc_opts);
    index_osp_additions->tune_options(kc_opts);

    // Open the databases
    open(index_spo_deletions, base_file_name + "_spo_deletions", readonly);
    open(index_pos_deletions, base_file_name + "_pos_deletions", readonly);
    open(index_osp_deletions, base_file_name + "_osp_deletions", readonly);
    open(index_spo_additions, base_file_name + "_spo_additions", readonly);
    open(index_pos_additions, base_file_name + "_pos_additions", readonly);
    open(index_osp_additions, base_file_name + "_osp_additions", readonly);
    if (!count_additions->open(base_file_name + "_count_additions", (readonly ? HashDB::OREADER : (HashDB::OWRITER | HashDB::OCREATE)) | HashDB::ONOREPAIR)) {
        cerr << "Open addition count tree error: " << count_additions->error().name() << endl;
    }
    if (temp_count_additions != NULL) {
        if (!temp_count_additions->open(base_file_name + "_count_additions.tmp",
                                        (readonly ? HashDB::OREADER : (HashDB::OWRITER | HashDB::OCREATE)) |
                                        HashDB::ONOREPAIR)) {
            cerr << "Open addition count tree error: " << temp_count_additions->error().name() << endl;
        }
    }
}

TripleStore::~TripleStore() {
    // Close the databases
    close(index_spo_deletions, "spo_deletions");
    close(index_pos_deletions, "pos_deletions");
    close(index_osp_deletions, "osp_deletions");
    close(index_spo_additions, "spo_additions");
    close(index_pos_additions, "pos_additions");
    close(index_osp_additions, "osp_additions");

    if (!count_additions->close()) {
        cerr << "Close addition count tree error: " << count_additions->error().name() << endl;
    }
    delete count_additions;
    if (temp_count_additions != NULL) {
        string path = temp_count_additions->path();
        if (!temp_count_additions->close()) {
            cerr << "Close temp addition count tree error: " << temp_count_additions->error().name() << endl;
        }
        delete temp_count_additions;
        std::remove(path.c_str());
    }

    delete spo_comparator;
    delete pos_comparator;
    delete osp_comparator;
    delete element_comparator;
}

void TripleStore::open(TreeDB* db, string name, bool readonly) {
    db->tune_map(KC_MEMORY_MAP_SIZE);
    //db->tune_buckets(1LL * 1000 * 1000);
    db->tune_page_cache(KC_PAGE_CACHE_SIZE);
    db->tune_defrag(8);
    if (!db->open(name, (readonly ? TreeDB::OREADER : (TreeDB::OWRITER | TreeDB::OCREATE)) | TreeDB::ONOREPAIR)) {
        cerr << "open " << name << " error: " << db->error().name() << endl;
    }
}

void TripleStore::close(TreeDB* db, string name) {
    if (!db->close()) {
        cerr << "close " << name << " error: " << db->error().name() << endl;
    }
    delete db;
}

TreeDB* TripleStore::getAdditionsTree(Triple triple_pattern) {
    bool s = triple_pattern.get_subject() > 0;
    bool p = triple_pattern.get_predicate() > 0;
    bool o = triple_pattern.get_object() > 0;

    if( s &  p &  o) return index_spo_additions;
    if( s &  p & !o) return index_spo_additions;
    if( s & !p &  o) return index_osp_additions;
    if( s & !p & !o) return index_spo_additions;
    if(!s &  p &  o) return index_pos_additions;
    if(!s &  p & !o) return index_pos_additions;
    if(!s & !p &  o) return index_osp_additions;
    /*if(!s & !p & !o) */return index_spo_additions;
}

TreeDB* TripleStore::getDefaultAdditionsTree() {
    return index_spo_additions;
}

TreeDB* TripleStore::getDeletionsTree(Triple triple_pattern) {
    bool s = triple_pattern.get_subject() > 0;
    bool p = triple_pattern.get_predicate() > 0;
    bool o = triple_pattern.get_object() > 0;

    if( s &  p &  o) return index_spo_deletions;
    if( s &  p & !o) return index_spo_deletions;
    if( s & !p &  o) return index_osp_deletions;
    if( s & !p & !o) return index_spo_deletions;
    if(!s &  p &  o) return index_pos_deletions;
    if(!s &  p & !o) return index_pos_deletions;
    if(!s & !p &  o) return index_osp_deletions;
    /*if(!s & !p & !o) */return index_spo_deletions;
}

TreeDB* TripleStore::getDefaultDeletionsTree() {
    return index_spo_deletions;
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, const PatchTreeAdditionValue* value, DB::Cursor* cursor) {
    size_t key_size, value_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);

    if (cursor != NULL) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo_additions->set(raw_key, key_size, raw_value, value_size);
    }
    index_pos_additions->set(raw_key, key_size, raw_value, value_size);
    index_osp_additions->set(raw_key, key_size, raw_value, value_size);

    free((char*) raw_key);
    free((char*) raw_value);

    // Flush db to disk
    if (++flush_counter_additions > FLUSH_TRIPLES_COUNT) {
        index_spo_additions->synchronize();
        index_pos_additions->synchronize();
        index_osp_additions->synchronize();
        flush_counter_additions = 0;
    }
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor) {
    // Look up the value for the given triple key in the tree.
    PatchTreeAdditionValue value;
    if (!ignore_existing) {
        // We assume that are indexes are sane, we only check one of them
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == NULL ? index_spo_additions->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
        if (raw_value) {
            value.deserialize(raw_value, value_size);
            delete[] raw_value;
        }
        free((char*) raw_key);
    }
    value.add(patch_id);
    if (local_change) {
        value.set_local_change(patch_id);
    }

    insertAdditionSingle(key, &value);
    if (!local_change) increment_addition_counts(patch_id, *key);
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchTreeDeletionValue* value, const PatchTreeDeletionValueReduced* value_reduced, DB::Cursor* cursor) {
    size_t key_size, value_size, value_reduced_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);
    const char *raw_value_reduced = value_reduced->serialize(&value_reduced_size);

    if (cursor != NULL) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo_deletions->set(raw_key, key_size, raw_value, value_size);
    }
    index_pos_deletions->set(raw_key, key_size, raw_value_reduced, value_reduced_size);
    index_osp_deletions->set(raw_key, key_size, raw_value_reduced, value_reduced_size);

    free((char*) raw_key);
    free((char*) raw_value);

    // Flush db to disk
    if (++flush_counter_deletions > FLUSH_TRIPLES_COUNT) {
        index_spo_deletions->synchronize();
        index_pos_deletions->synchronize();
        index_osp_deletions->synchronize();
        flush_counter_deletions = 0;
    }
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchPositions& patch_positions, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor) {
    PatchTreeDeletionValue deletion_value;
    if (!ignore_existing) {
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == NULL ? index_spo_deletions->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
        if (raw_value) {
            deletion_value.deserialize(raw_value, value_size);
            delete[] raw_value;
        }
        free((char*) raw_key);
    }
    PatchTreeDeletionValueElement element = PatchTreeDeletionValueElement(patch_id, patch_positions);
    if (local_change) {
        element.set_local_change();
    }
    deletion_value.add(element);

    // Convert our full deletion value to a reduced one
    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
    insertDeletionSingle(key, &deletion_value, &deletion_value_reduced);
}

PatchTreeKeyComparator *TripleStore::get_spo_comparator() const {
    return spo_comparator;
}

PatchElementComparator *TripleStore::get_element_comparator() const {
    return element_comparator;
}

DictionaryManager *TripleStore::get_dict_manager() const {
    return dict;
}

void TripleStore::increment_addition_counts(const int patch_id, const Triple &triple) {
    increment_addition_count(TripleVersion(patch_id, Triple(triple.get_subject(), triple.get_predicate(), 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(triple.get_subject(), 0                     , 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(triple.get_subject(), 0                     , triple.get_object())));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , triple.get_predicate(), 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , triple.get_predicate(), triple.get_object())));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , 0                     , 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , 0                     , triple.get_object())));
}

void TripleStore::increment_addition_count(const TripleVersion& triple_version) {
    size_t _;
    char raw_key[sizeof(TripleVersion)];
    char* raw_value;
    bool was_present = false;
    memcpy(raw_key, &triple_version, sizeof(TripleVersion));

    raw_value = temp_count_additions->get(raw_key, sizeof(TripleVersion), &_);
    PatchPosition pos = 0;
    if (raw_value != NULL) {
        was_present = true;
        memcpy(&pos, raw_value, sizeof(PatchPosition));
    } else {
        raw_value = (char*) malloc(sizeof(TripleVersion));
    }
    pos++;
    memcpy(raw_value, &pos, sizeof(PatchPosition));
    temp_count_additions->set(raw_key, sizeof(TripleVersion), raw_value, sizeof(PatchPosition));

    if (was_present) {
        delete[] raw_value;
    } else {
        free(raw_value);
    }
}

PatchPosition TripleStore::get_addition_count(const int patch_id, const Triple &triple) {
    size_t ksp, vsp;
    TripleVersion key(patch_id, triple);
    const char* kbp = key.serialize(&ksp);
    const char* vbp = count_additions->get(kbp, ksp, &vsp);
    PatchPosition count = 0;
    if (vbp != NULL) {
        memcpy(&count, vbp, sizeof(PatchPosition));
    }
    return count;
}

long TripleStore::flush_addition_counts() {
    size_t ksp, vsp;
    PatchPosition count = 0;
    HashDB::Cursor* cursor = temp_count_additions->cursor();
    cursor->jump();
    long added = 0;
    while (cursor->step()) {
        const char* vbp;
        const char* kbp = cursor->get(&ksp, &vbp, &vsp, false);
        memcpy(&count, vbp, sizeof(PatchPosition));
        if (count >= MIN_ADDITION_COUNT) {
            count_additions->set(kbp, ksp, vbp, vsp);
            added++;
        }
    }
    delete cursor;
    count_additions->synchronize();
    temp_count_additions->clear();

    return added;
}


void TripleStore::purge_versions(int version, DictionaryManager* dict) {
    purge_version_in_addition_tree(version, index_spo_additions, dict, true);
    purge_version_in_addition_tree(version, index_pos_additions, dict, false);
    purge_version_in_addition_tree(version, index_osp_additions, dict, false);
    purge_versions_in_deletions(version, dict, true);
}

void TripleStore::purge_versions_in_deletions(int version, DictionaryManager* dict, bool write_to_file){
    DB::Cursor* spo_cursor = index_spo_deletions->cursor();
    DB::Cursor* pos_cursor = index_pos_deletions->cursor();
    DB::Cursor* osp_cursor = index_osp_deletions->cursor();

//    print_tree(index_spo_deletions, false, dict);
//    print_tree(index_osp_deletions, false, dict);
//    print_tree(index_pos_deletions, false, dict);

    spo_cursor->jump();
    pos_cursor->jump();
    osp_cursor->jump();

    size_t ksp, vsp;
    const char *kbp, *vbp;


    while(kbp = spo_cursor->get(&ksp, &vbp, &vsp, true)){

        pos_cursor->step();
        osp_cursor->step();

        Triple triple;
        triple.deserialize(kbp, ksp);

        PatchTreeDeletionValue deletionValue;
        deletionValue.deserialize(vbp, vsp);

        if(write_to_file){
            // write versions
            auto elements = deletionValue.getElements();
            // versions are assumed to be incremented, so previous version = current version - 1
            for (int i = 0; i < elements.size(); i++) {
                if (elements[i].get_patch_id() >= version) { //marked for erasion
                    if(i==0 || elements[i - 1].get_patch_id() + 1 != elements[i].get_patch_id()){
                        // write to file
                        // TODO change to write to file
                        std::cout << "file: " << elements[i].get_patch_id() << " triple: " << triple.to_string(*dict) << " -" << std::endl;
                    }
                }
            }
        }

        // erase versions
        int original_size = deletionValue.get_size();
        deletionValue.remove_versions(version);

        bool remove = deletionValue.get_size() == 0;
        bool replace = deletionValue.get_size() < original_size;
        // replace or remove
        if(remove){
            index_spo_deletions->remove(kbp, ksp);
            index_pos_deletions->remove(kbp, ksp); //also remove from other trees, since they have reduced deletion values
            index_osp_deletions->remove(kbp, ksp);
        }
        else if (replace){
            vbp = deletionValue.serialize(&vsp);
            index_spo_deletions->replace(kbp, ksp, vbp, vsp);
        }
    }

    delete[] kbp;
    delete[] vbp;
    delete spo_cursor;
    delete pos_cursor;
    delete osp_cursor;

//    print_tree(index_spo_deletions, false, dict);
//    print_tree(index_osp_deletions, false, dict);
//    print_tree(index_pos_deletions, false, dict);


}

void TripleStore::extract_additions(DictionaryManager* dict, std::string path){
    DB::Cursor* cur = index_spo_additions->cursor();
    cur->jump();
    size_t ksp, vsp;
    const char *kbp;
    const char *vbp;
    while (kbp = cur->get(&ksp, &vbp, &vsp, true)) {
        Triple triple;
        triple.deserialize(kbp, ksp);

        PatchTreeAdditionValue additionValue;
        additionValue.deserialize(vbp, vsp);
        // write versions
        std::vector<int> patches = additionValue.getPatches();
        // versions are assumed to be incremented, so previous version = current version - 1
        for (int i = 0; i < patches.size(); i++) {
            if (i == 0 || patches[i - 1] + 1 != patches[i]) {
                ofstream file;

                boost::filesystem::path dir(path + "/" + to_string(patches[i]));

                if(!(boost::filesystem::exists(dir))){
                    boost::filesystem::create_directory(dir);
                }

                file.open((path + "/" + to_string(patches[i]) + "/main.nt.additions.txt").c_str(), std::ios::app); //TODO windows support
                file << triple.to_string_bear(*dict) << std::endl;
                file.close();
            }
        }
    }
    delete[] kbp;
    delete[] vbp;
    delete cur;

}

void TripleStore::extract_deletions(DictionaryManager* dict, std::string path){
    DB::Cursor* cur = index_spo_deletions->cursor();
    cur->jump();
    size_t ksp, vsp;
    const char *kbp;
    const char *vbp;
    while (kbp = cur->get(&ksp, &vbp, &vsp, true)) {
        Triple triple;
        triple.deserialize(kbp, ksp);

        PatchTreeDeletionValue deletionValue;
        deletionValue.deserialize(vbp, vsp);
        // write versions
        auto elements = deletionValue.getElements();
        // versions are assumed to be incremented, so previous version = current version - 1
        for (int i = 0; i < elements.size(); i++) {
            if(i==0 || elements[i - 1].get_patch_id() + 1 != elements[i].get_patch_id()){
                // write to file

                ofstream file;

                boost::filesystem::path dir(path + "/" + to_string(elements[i].get_patch_id()));

                if(!(boost::filesystem::exists(dir))){
                    boost::filesystem::create_directory(dir);
                }

                file.open((path + "/" + to_string(elements[i].get_patch_id()) + "/main.nt.deletions.txt").c_str(), std::ios::app); //TODO windows support
                file << triple.to_string_bear(*dict) << std::endl;
                file.close();
            }
        }
    }
    delete[] kbp;
    delete[] vbp;
    delete cur;

}

void TripleStore::purge_version_in_addition_tree(int version, TreeDB* tree, DictionaryManager* dict, bool write_to_file){
    DB::Cursor* cur = tree->cursor();

//    print_tree(tree, true, dict);


    cur->jump();
    size_t ksp, vsp;
    const char *kbp;
    const char *vbp;
    while (kbp = cur->get(&ksp, &vbp, &vsp, true)) {
        Triple triple;
        triple.deserialize(kbp, ksp);

        PatchTreeAdditionValue additionValue;
        additionValue.deserialize(vbp, vsp);

        if (write_to_file) {
            // write versions
            std::vector<int> patches = additionValue.getPatches();
            // versions are assumed to be incremented, so previous version = current version - 1
            for (int i = 0; i < patches.size(); i++) {
                if (patches[i] >= version) { //marked for erasion
                    if (i == 0 || patches[i - 1] + 1 != patches[i]) {
                        // write to file
                        // TODO change to write to file
                        std::cout << "file: " << patches[i] << " triple: " << triple.to_string(*dict) << " +"
                                  << std::endl;
                    }
                }
            }

        }

        // erase versions
        int original_size = additionValue.get_size();
        additionValue.remove_versions(version);

        bool remove = additionValue.get_size() == 0;
        bool replace = additionValue.get_size() < original_size;


        // replace or remove
        if (remove) {
            tree->remove(kbp, ksp); //should this be done at the end of the while loop?
        } else if (replace) {
            vbp = additionValue.serialize(&vsp);
            tree->replace(kbp, ksp, vbp, vsp);
        }
    }


    delete[] kbp;
    delete[] vbp;
    delete cur;

//    print_tree(tree, true, dict);

}

void TripleStore::print_tree(TreeDB* tree, bool addition, DictionaryManager* dict) {
    DB::Cursor* cur = tree->cursor();
    cur->jump();
    size_t ksp, vsp;
    const char *kbp;
    const char *vbp;

    while (kbp = cur->get(&ksp, &vbp, &vsp, true)) {
        Triple triple;
        triple.deserialize(kbp, ksp);

        if (addition){
            PatchTreeAdditionValue additionValue;
            additionValue.deserialize(vbp, vsp);
            std::cout << triple.to_string(*dict) << ":" << additionValue.to_string() << std::endl;
        }
        else {
            PatchTreeDeletionValue deletionValue;
            deletionValue.deserialize(vbp, vsp);
            std::cout << triple.to_string(*dict) << ":" << deletionValue.to_string() << std::endl;
        }
    }

    delete[] kbp;
    delete[] vbp;
    delete cur;
}

void TripleStore::print_trees(DictionaryManager* dict) {
    std::cout << "SPO addition tree" << std::endl;
    print_tree(index_spo_additions, true, dict);
    std::cout << "POS addition tree" << std::endl;
    print_tree(index_pos_additions, true, dict);
    std::cout << "OSP addition tree" << std::endl;
    print_tree(index_osp_additions, true, dict);
    std::cout << "SPO deletion tree" << std::endl;
    print_tree(index_spo_deletions, false, dict);
    std::cout << "POS deletion tree" << std::endl;
    print_tree(index_pos_deletions, false, dict);
    std::cout << "OSP deletion tree" << std::endl;
    print_tree(index_osp_deletions, false, dict);
}
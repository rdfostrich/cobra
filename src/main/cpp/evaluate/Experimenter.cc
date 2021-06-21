//
// Created by thibault on 13/03/18.
//

#include <dirent.h>
#include "Experimenter.h"
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>
#include "../snapshot/combined_triple_iterator.h"
#include "../simpleprogresslistener.h"

void
Experimenter::populate_snapshot(int start_id, int end_id, ProgressListener *progressListener, Controller controller) {
    std::smatch base_match;
    std::regex regex_snapshot("([a-z0-9]*).nt.snapshot.txt");
    std::regex regex_additions("([a-z0-9]*).nt.additions.txt");
    std::regex regex_deletions("([a-z0-9]*).nt.deletions.txt");
    DIR *dir;
    struct dirent *ent;

    for(int snapshot_id = start_id; snapshot_id<=end_id;snapshot_id++) {
        StopWatch st;
        CombinedTripleIterator *it_snapshot = new CombinedTripleIterator();
        //read snapshot
        std::string path = patchesBasePatch + k_path_separator + to_string(snapshot_id);
        if ((dir = opendir(path.c_str())) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                std::string filename = std::string(ent->d_name);
                std::string full_path = path + k_path_separator + filename;
                if (filename != "." && filename != ".." && !std::regex_match(filename, base_match, regex_additions) && !std::regex_match(filename, base_match, regex_deletions)) {
                    std::regex_match(filename, base_match, regex_snapshot);
                    NOTIFYMSG(progressListener, ("FILE: " + full_path + "\n").c_str());
                    it_snapshot->appendIterator(get_from_file(full_path));
                }
            }
            closedir(dir);
        }
        //append snapshot
        NOTIFYMSG(progressListener, "\nCreating snapshot...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT *hdt = controller.get_snapshot_manager()->create_snapshot(snapshot_id, it_snapshot, BASEURI, progressListener);
        std::cout.clear();

        long long added = hdt->getTriples()->getNumberOfElements();
        long long duration = st.stopReal() / 1000;
        if (duration == 0) duration = 1; // Avoid division by 0
        long long rate = added / duration;
        std::ifstream::pos_type accsize = patchstore_size(&controller);
        cout << snapshot_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;
        performance_file << snapshot_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;

        delete it_snapshot;
    }
//    performance_file << "ended populate_snapshot: " << to_string(start_id) << "-" << to_string(end_id) << endl;
}

void Experimenter::populate_patch_tree(int start_id, int end_id, ProgressListener *progressListener, bool copy, int copy_id,
                                       Controller controller) {
    if(copy_id == -1)
        copy_id = (end_id - start_id)/2;
    int snapshot_id;
    DictionaryManager *dict;

    // forward patch tree
    if(start_id <= end_id){
        snapshot_id = start_id - 1;
        controller.get_snapshot_manager()->get_snapshot(snapshot_id); //load snapshot and dict
        dict = controller.get_snapshot_manager()->get_dictionary_manager(snapshot_id);
        controller.get_patch_tree_manager()->construct_next_patch_tree(start_id, dict);
        for(int patch_id = start_id; patch_id <= end_id; patch_id++){
            StopWatch st;
            NOTIFYMSG(progressListener, "Loading patch...\n");
            PatchElementIteratorCombined* it_patch = load_patch(patch_id, dict, progressListener);
            NOTIFYMSG(progressListener, ("Version " + to_string(patch_id) + "\n").c_str());

            long long added;
            NOTIFYMSG(progressListener, "\nAppending patch...\n");
            controller.append(it_patch, patch_id, dict, false, progressListener);
            added = it_patch->getPassed();

            if(copy && patch_id == copy_id){
                dict->save();
//                save_controller();
                controller.get_snapshot_manager()->load_snapshot(snapshot_id);
                controller.copy_patch_tree_files(basePath, start_id);
                dict = controller.get_snapshot_manager()->get_dictionary_manager(snapshot_id);
                controller.get_patch_tree_manager()->load_patch_tree(snapshot_id + 1, dict);
                NOTIFYMSG(progressListener, ("Copied version " + to_string(patch_id) + "\n").c_str());
            }
            std::ifstream::pos_type accsize = patchstore_size(&controller);
            long long duration = st.stopReal() / 1000;
            if (duration == 0) duration = 1; // Avoid division by 0
            long long rate = added / duration;
            if(copy && patch_id == copy_id){
                copy = false; // to prevent double copying
                std::ifstream::pos_type temp_accsize = temp_patchstore_size(&controller);
                cout << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;
                performance_file << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << "," << temp_accsize << endl;
            }
            else{
                cout << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;
                performance_file << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;
            }
            delete it_patch;
        }
    }
        // reverse patch tree
    else{
        snapshot_id = start_id + 1;
        controller.get_snapshot_manager()->get_snapshot(snapshot_id); //load snapshot and dict
        dict = controller.get_snapshot_manager()->get_dictionary_manager(snapshot_id);
        controller.get_patch_tree_manager()->construct_next_patch_tree(start_id, dict);
        for(int patch_id = start_id; patch_id >= end_id; patch_id--){
            StopWatch st;
            NOTIFYMSG(progressListener, "Loading patch...\n");
            PatchElementIteratorCombined* it_patch = reverse_load_patch(patch_id + 1, dict, progressListener);
            NOTIFYMSG(progressListener, ("Version " + to_string(patch_id) + "\n").c_str());

            long long added;
            NOTIFYMSG(progressListener, "\nReverse Appending patch...\n");
            controller.reverse_append(it_patch, patch_id, dict, false, progressListener);
            added = it_patch->getPassed();

            long long duration = st.stopReal() / 1000;
            if (duration == 0) duration = 1; // Avoid division by 0
            long long rate = added / duration;
            std::ifstream::pos_type accsize = patchstore_size(&controller);
            cout << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;
            performance_file << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;

            delete it_patch;
        }
    }

//    performance_file << "ended populate_patch_tree: " << to_string(start_id) << "-" << to_string(end_id) << endl;

}


IteratorTripleString* Experimenter::get_from_file(string file) {
    return new RDFParserNtriples(file.c_str(), NTRIPLES);
}

PatchElementIteratorCombined* Experimenter::load_patch(int patch_id, DictionaryManager *dict, ProgressListener *progressListener){
    std::smatch base_match;
    std::regex regex_additions("([a-z0-9]*).nt.additions.txt");
    std::regex regex_deletions("([a-z0-9]*).nt.deletions.txt");
    std::regex regex_snapshot("([a-z0-9]*).nt.snapshot.txt");
    DIR *dir;
    struct dirent *ent;
    PatchElementIteratorCombined* it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    string path = patchesBasePatch + k_path_separator + to_string(patch_id);
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = string(ent->d_name);
            string full_path = path + k_path_separator + filename;
            if (filename != "." && filename != ".." && !std::regex_match(filename, base_match, regex_snapshot)) {
                NOTIFYMSG(progressListener, ("FILE: " + full_path + "\n").c_str());
                bool additions = std::regex_match(filename, base_match, regex_additions);
                bool deletions = std::regex_match(filename, base_match, regex_deletions);
                IteratorTripleString *subIt = get_from_file(full_path);
                PatchElementIteratorTripleStrings* patchIt = new PatchElementIteratorTripleStrings(dict, subIt, additions);
                it_patch->appendIterator(patchIt);
            }
        }
        closedir(dir);
    }
    return it_patch;
}

PatchElementIteratorCombined* Experimenter::reverse_load_patch(int patch_id, DictionaryManager *dict, ProgressListener *progressListener){
    std::smatch base_match;
    std::regex regex_additions("([a-z0-9]*).nt.additions.txt");
    std::regex regex_deletions("([a-z0-9]*).nt.deletions.txt");
    std::regex regex_snapshot("([a-z0-9]*).nt.snapshot.txt");
    DIR *dir;
    struct dirent *ent;
    PatchElementIteratorCombined* it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    string path = patchesBasePatch + k_path_separator + to_string(patch_id);
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = string(ent->d_name);
            string full_path = path + k_path_separator + filename;
            if (filename != "." && filename != ".." && !std::regex_match(filename, base_match, regex_snapshot)) {
                NOTIFYMSG(progressListener, ("FILE: " + full_path + "\n").c_str());
                bool additions = std::regex_match(filename, base_match, regex_additions);
                bool deletions = std::regex_match(filename, base_match, regex_deletions);
                IteratorTripleString *subIt = get_from_file(full_path);
                PatchElementIteratorTripleStrings* patchIt = new PatchElementIteratorTripleStrings(dict, subIt, deletions);
                it_patch->appendIterator(patchIt);
            }
        }
        closedir(dir);
    }
    return it_patch;
}

std::ifstream::pos_type Experimenter::temp_patchstore_size(Controller* controller) {
    long size = 0;

    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    while(itP != patches.end()) {
        int id = itP->first;

        // temp files
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "spo_deletions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "pos_deletions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "pso_deletions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "sop_deletions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "osp_deletions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "spo_additions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "pos_additions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "pso_additions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "sop_additions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "osp_additions"));
        size += filesize(basePath + TEMP_PATCHTREE_FILENAME(id, "count_additions"));
        size += filesize(basePath + TEMP_METADATA_FILENAME_BASE(id));

        itP++;
    }
    return size;
}

std::ifstream::pos_type Experimenter::patchstore_size(Controller* controller) {
    long size = 0;

    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    while(itP != patches.end()) {
        int id = itP->first;
        size += filesize(basePath + PATCHTREE_FILENAME(id, "spo_deletions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "pos_deletions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "pso_deletions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "sop_deletions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "osp_deletions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "spo_additions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "pos_additions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "pso_additions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "sop_additions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "osp_additions"));
        size += filesize(basePath + PATCHTREE_FILENAME(id, "count_additions"));
        size += filesize(basePath + METADATA_FILENAME_BASE(id));

        itP++;
    }
//
    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    controller->get_snapshot_manager()->get_dictionary_manager(0)->save();
    std::map<int, HDT*>::iterator itS = snapshots.begin();
    while(itS != snapshots.end()) {
        int id = itS->first;
        size += filesize(basePath + SNAPSHOT_FILENAME_BASE(id));
        size += filesize(basePath + (SNAPSHOT_FILENAME_BASE(id) + ".index"));
        size += filesize(basePath + (PATCHDICT_FILENAME_BASE(id)));
        itS++;
    }

    return size;
}

std::ifstream::pos_type Experimenter::filesize(string file) {
    return std::ifstream(file.c_str(), std::ifstream::ate | std::ifstream::binary).tellg();
}

Experimenter::Experimenter(string performance_filename, string basePath, string patchesBasePatch, ProgressListener *progressListener) {
    controller = new Controller(basePath, TreeDB::TCOMPRESS);
    Experimenter::basePath = basePath;
    Experimenter::patchesBasePatch = patchesBasePatch;
    performance_file.open(performance_filename, std::fstream::app);
}

Experimenter::~Experimenter() {
//    controller->cleanup(basePath, controller);
    performance_file.close();
    delete controller;
}

Controller *Experimenter::getController() const {
    return controller;
}

void Experimenter::print_store_structure() {
    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    for (auto& kv : controller->get_snapshot_manager()->get_snapshots()) {
        int snapshot_id = kv.first;
        controller->get_snapshot_manager()->load_snapshot(snapshot_id); //load
        DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
        auto it_reverse = patches.find(snapshot_id - 1);
        if(it_reverse != patches.end()){
            if(it_reverse->second == NULL){
                it_reverse->second = controller->get_patch_tree_manager()->load_patch_tree(it_reverse->first, dict); //load
            }
            std::cout << "P" << to_string(it_reverse->second->get_min_patch_id()) << "_" << to_string(it_reverse->second->get_max_patch_id()) << " -> ";
        }
        std::cout << "S_" << to_string(snapshot_id);
        auto it_forward = patches.find(snapshot_id + 1);
        if(it_forward != patches.end()){
            if(it_forward->second == NULL){
                it_forward->second = controller->get_patch_tree_manager()->load_patch_tree(it_forward->first, dict); //load
            }
            std::cout << " <- P" << to_string(it_forward->second->get_min_patch_id()) << "_" << to_string(it_forward->second->get_max_patch_id());
        }
    }

}

long Experimenter::test_lookup_version_materialized(Controller* controller, Triple triple_pattern, int offset, int patch_id, int limit) {
    StopWatch st;
    TripleIterator* ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
    Triple t;
    while((limit == -2 || limit-- > 0) && ti->next(&t));
    return st.stopReal() / 1000;
}

long Experimenter::test_lookup_delta_materialized(Controller* controller, Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit) {
    StopWatch st;
    TripleDeltaIterator* ti = controller->get_delta_materialized(triple_pattern, offset, patch_id_start, patch_id_end);
    TripleDelta t;
    while((limit == -2 || limit-- > 0) && ti->next(&t));
    return st.stopReal() / 1000;
}

long Experimenter::test_lookup_version(Controller* controller, Triple triple_pattern, int offset, int limit) {
    StopWatch st;
    TripleVersionsIterator* ti = controller->get_version(triple_pattern, offset);
    TripleVersions t;
    while((limit == -2 || limit-- > 0) && ti->next(&t));
    return st.stopReal() / 1000;
}
/**
 * Assumes temp patch tree is already made
 * Assumes end snapshot already inserted
 */
void Experimenter::reverse(int start_snapshot, int end_snapshot, ProgressListener *progressListener){
    StopWatch st;
    controller->replace_patch_tree(basePath, start_snapshot + 1);
    populate_patch_tree(end_snapshot - 1, ((end_snapshot - start_snapshot) / 2 + 1), progressListener, false, -1,
                        Controller(basePath, TreeDB::TCOMPRESS));
    long duration = st.stopReal() / 1000;
    std::ifstream::pos_type accsize = patchstore_size(controller);
    performance_file << "replaced " << duration << "," << accsize << endl;
    NOTIFYMSG(progressListener, ("Replaced Patch Tree " + std::to_string(start_snapshot + 1)).c_str())
}

/**
 * removes chain forward chain link and inserts in reverse
 */
void Experimenter::reverse_opt(int start_temp_snapshot_id, int end_snapshot_id, ProgressListener *progressListener){
    StopWatch st;
    controller->remove_forward_chain(basePath, start_temp_snapshot_id);
    populate_patch_tree(end_snapshot_id - 1, start_temp_snapshot_id, progressListener, false, -1,
                        Controller(basePath, TreeDB::TCOMPRESS));
    long duration = st.stopReal() / 1000;
    std::ifstream::pos_type accsize = patchstore_size(controller);
    performance_file << "replaced " << duration << "," << accsize << endl;
    NOTIFYMSG(progressListener, ("Replaced Patch Tree " + std::to_string(start_temp_snapshot_id)).c_str())
}


long long Experimenter::measure_lookup_version_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleIterator* ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
        // Dummy loop over iterator
        Triple t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_subject(dict);
            t.get_predicate(dict);
            t.get_object(dict);
            result_count++;
        };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Experimenter::measure_count_version_materialized(Triple triple_pattern, int patch_id, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_version_materialized_count(triple_pattern, patch_id, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Experimenter::measure_lookup_delta_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleDeltaIterator *ti = controller->get_delta_materialized(triple_pattern, offset, patch_id_start,
                                                                     patch_id_end);
        TripleDelta t;
        while ((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_triple()->get_subject(dict);
            t.get_triple()->get_predicate(dict);
            t.get_triple()->get_object(dict);
            result_count++;
        };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Experimenter::measure_count_delta_materialized(Triple triple_pattern, int patch_id_start, int patch_id_end, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Experimenter::measure_lookup_version(Dictionary& dict, Triple triple_pattern, int offset, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleVersionsIterator* ti = controller->get_partial_version(triple_pattern, offset);
        TripleVersions t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_triple()->get_subject(dict);
            t.get_triple()->get_predicate(dict);
            t.get_triple()->get_object(dict);
            result_count++;
        };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Experimenter::measure_count_version(Triple triple_pattern, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_partial_version_count(triple_pattern, true);
        total += st.stopReal();
    }
    return total / replications;
}

void Experimenter::test_lookup(int patch_count, string s, string p, string o, int replications, int offset, int limit) {
    controller->get_snapshot_manager()->load_all_snapshots();
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);


    //warm up
    Triple default_tp("", "", "", dict);
    HDT * hdt = controller->get_snapshot_manager()->get_snapshot(patch_count/2);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(hdt, default_tp, 0);
    int count = 0;
    while (snapshot_it->hasNext() && count < 100) {
        snapshot_it->next();
        //cout << "Result Warmup: " << triple->getSubject() << ", " << triple->getPredicate() << ", " << triple->getObject() << endl;
        count++;
    }
    PatchTreeIterator it = controller->get_patch_tree_manager()->get_patch_tree(patch_count / 2 - 1, dict)->iterator(&default_tp);
    PatchTreeKey key;
    PatchTreeValue val;
    count = 0;
    while (it.next(&key, &val) && count < 100) {
        //cout << "Result Warmup: " << triple->getSubject() << ", " << triple->getPredicate() << ", " << triple->getObject() << endl;
        count++;
    }
    PatchTreeKey key2;
    PatchTreeValue val2;
    PatchTreeIterator it2 = controller->get_patch_tree_manager()->get_patch_tree(patch_count / 2 + 1, dict)->iterator(&default_tp);
    count = 0;
    while (it2.next(&key2, &val2) && count < 100) {
        //cout << "Result Warmup: " << triple->getSubject() << ", " << triple->getPredicate() << ", " << triple->getObject() << endl;
        count++;
    }
    //warm up finished

    Triple triple_pattern(s, p, o, dict);
    performance_file << "---PATTERN START: " << triple_pattern.to_string(*dict) << endl;

    performance_file << "--- ---VERSION MATERIALIZED" << endl;
    performance_file << "patch,offset,limit,count-ms,lookup-mus,results" << endl;
    for(int i = patch_count/2; i < patch_count; i++) {
//        DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(i);
//        Triple triple_pattern(s, p, o, dict);

        int result_count1 = 0;
        long dcount = measure_count_version_materialized(triple_pattern, i, replications);
        long d1 = measure_lookup_version_materialized(*dict, triple_pattern, offset, i, limit, replications, result_count1);
        performance_file << "" << i << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
    }
    for(int i = patch_count/2 - 1; i >= 0 ; i--) {
//        DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(i);
//        Triple triple_pattern(s, p, o, dict);

        int result_count1 = 0;
        long dcount = measure_count_version_materialized(triple_pattern, i, replications);
        long d1 = measure_lookup_version_materialized(*dict, triple_pattern, offset, i, limit, replications, result_count1);
        performance_file << "" << i << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
    }
    // only one dict supported for delta materialized
    performance_file << "--- ---DELTA MATERIALIZED" << endl;
    performance_file << "patch_start,patch_end,offset,limit,count-ms,lookup-mus,results" << endl;
    int i = 0;
    //for(int i = 0; i < patch_count; i++) {
        for(int j = i + 1; j < patch_count; j++) {
            int result_count1 = 0;
            long dcount = measure_count_delta_materialized(triple_pattern, i, j, replications);
            long d1 = measure_lookup_delta_materialized(*dict, triple_pattern, offset, i, j, limit, replications, result_count1);
            performance_file << "" << i << "," << j << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
        }
    //}

    performance_file << "--- ---VERSION" << endl;
    performance_file << "offset,limit,count-ms,lookup-mus,results" << endl;
    int result_count1 = 0;
    long dcount = measure_count_version(triple_pattern, replications);
    long d1 = measure_lookup_version(*dict, triple_pattern, offset, limit, replications, result_count1);
    performance_file << "" << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
}

void Experimenter::extract_changeset(Controller controller, int patchtree_id, string path_to_files) {
    StopWatch st;
    controller.extract_changeset(patchtree_id, path_to_files);
    auto total = st.stopReal() / 1000;
    performance_file << "extract duration: " << total << endl;

}

void Experimenter::remove_forward_delta_chain(Controller controller, string basePath, int snapshot_id) {
    StopWatch st;
    controller.remove_forward_chain(basePath,snapshot_id);
    auto total = st.stopReal() / 1000;
    performance_file << "remove duration: " << total << endl;
}

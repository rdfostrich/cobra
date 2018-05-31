#include <iostream>
#include <dirent.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>

#include "../snapshot/combined_triple_iterator.h"

#include "evaluator.h"
#include "../simpleprogresslistener.h"

void Evaluator::init(string basePath, string patchesBasePatch, int startIndex, int endIndex, ProgressListener* progressListener) {
    controller = new Controller(basePath, TreeDB::TCOMPRESS);

    if(patchesBasePatch.compare("") != 0){ // do not reinsert
        cout << "---INSERTION START---" << endl;
        cout << "version,added,durationms,rate,accsize" << endl;
        DIR *dir;
        if ((dir = opendir(patchesBasePatch.c_str())) != NULL) {
            for (int i = startIndex; i <= endIndex; i++) {
                string versionname = to_string(i);
                NOTIFYMSG(progressListener, ("Version " + versionname + "\n").c_str());
                string path = patchesBasePatch + k_path_separator + versionname;
                populate_controller_with_version(patch_count++, path, progressListener);
            }
            closedir(dir);
        }
        cout << "---INSERTION END---" << endl;
    }
    else {
        patch_count = endIndex - startIndex + 1;
    }
}

void Evaluator::populate_controller_with_version(int patch_id, string path, ProgressListener* progressListener) {
    std::smatch base_match;
    std::regex regex_additions("([a-z0-9]*).nt.additions.txt");
    std::regex regex_deletions("([a-z0-9]*).nt.deletions.txt");
    std::regex regex_snapshot("([a-z0-9]*).nt.snapshot.txt");

    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    bool first = patch_id == 0;
    CombinedTripleIterator* it_snapshot = new CombinedTripleIterator();
    PatchElementIteratorCombined* it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));

    if (controller->get_max_patch_id() >= patch_id) {
        if (first) {
            NOTIFYMSG(progressListener, "Skipped constructing snapshot because it already exists, loaded instead.\n");
            controller->get_snapshot_manager()->load_snapshot(patch_id);
        } else {
            NOTIFYMSG(progressListener, "Skipped constructing patch because it already exists, loading instead...\n");
            DictionaryManager* dict_patch = controller->get_dictionary_manager(patch_id);
            if (controller->get_patch_tree_manager()->get_patch_tree(patch_id, dict_patch)->get_max_patch_id() < patch_id) {
                controller->get_patch_tree_manager()->load_patch_tree(patch_id, dict_patch);
            }
            NOTIFYMSG(progressListener, "Loaded!\n");
        }
        return;
    }

    DIR *dir;
    struct dirent *ent;
    StopWatch st;
    NOTIFYMSG(progressListener, "Loading patch...\n");
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = string(ent->d_name);
            string full_path = path + k_path_separator + filename;
            if (filename != "." && filename != "..") {
                bool additions = std::regex_match(filename, base_match, regex_additions);
                bool deletions = std::regex_match(filename, base_match, regex_deletions);
                bool snapshot = std::regex_match(filename, base_match, regex_snapshot);

                if (first && snapshot) {
                    it_snapshot->appendIterator(get_from_file(full_path));
                    NOTIFYMSG(progressListener, ("FILE: " + full_path + "\n").c_str());

                } else if(!first && (additions || deletions)) {
                    IteratorTripleString *subIt = get_from_file(full_path);
                    PatchElementIteratorTripleStrings* patchIt = new PatchElementIteratorTripleStrings(dict, subIt, additions);
                    it_patch->appendIterator(patchIt);
                    NOTIFYMSG(progressListener, ("FILE: " + full_path + "\n").c_str());

                }
            }
        }
        closedir(dir);
    }

    long long added;
    if (first) {
        NOTIFYMSG(progressListener, "\nCreating snapshot...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT* hdt = controller->get_snapshot_manager()->create_snapshot(0, it_snapshot, BASEURI, progressListener);
        std::cout.clear();
        added = hdt->getTriples()->getNumberOfElements();

    } else {
        NOTIFYMSG(progressListener, "\nAppending patch...\n");
        controller->append(it_patch, patch_id, dict, false, progressListener);
        added = it_patch->getPassed();
    }
    long long duration = st.stopReal() / 1000;
    if (duration == 0) duration = 1; // Avoid division by 0
    long long rate = added / duration;
    std::ifstream::pos_type accsize = patchstore_size(controller);
    cout << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;

    delete it_snapshot;
    delete it_patch;
}

std::ifstream::pos_type Evaluator::patchstore_size(Controller* controller) {
    long size = 0;

    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    while(itP != patches.end()) {
        int id = itP->first;
        size += filesize(PATCHTREE_FILENAME(id, "spo_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "pos_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "pso_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "sop_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "osp_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "spo_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "pos_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "pso_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "sop_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "osp_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "count_additions"));
        itP++;
    }

    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    controller->get_snapshot_manager()->get_dictionary_manager(0)->save();
    std::map<int, HDT*>::iterator itS = snapshots.begin();
    while(itS != snapshots.end()) {
        int id = itS->first;
        size += filesize(SNAPSHOT_FILENAME_BASE(id));
        size += filesize((SNAPSHOT_FILENAME_BASE(id) + ".index"));
        size += filesize((PATCHDICT_FILENAME_BASE(id)));
        itS++;
    }

    return size;
}

std::ifstream::pos_type Evaluator::filesize(string file) {
    return std::ifstream(file.c_str(), std::ifstream::ate | std::ifstream::binary).tellg();
}

IteratorTripleString* Evaluator::get_from_file(string file) {
    return new RDFParserNtriples(file.c_str(), NTRIPLES);
}

long long Evaluator::measure_lookup_version_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id, int limit, int replications, int& result_count) {
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

long long Evaluator::measure_count_version_materialized(Triple triple_pattern, int patch_id, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_version_materialized_count(triple_pattern, patch_id, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Evaluator::measure_lookup_delta_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications, int& result_count) {
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

long long Evaluator::measure_count_delta_materialized(Triple triple_pattern, int patch_id_start, int patch_id_end, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Evaluator::measure_lookup_version(Dictionary& dict, Triple triple_pattern, int offset, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleVersionsIterator* ti = controller->get_version(triple_pattern, offset);
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

long long Evaluator::measure_count_version(Triple triple_pattern, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_version_count(triple_pattern, true);
        total += st.stopReal();
    }
    return total / replications;
}

void
Evaluator::test_lookup(string s, string p, string o, int replications, int offset, int limit,
                       string performance_file_path) {
    ofstream performance_file;
    performance_file.open(performance_file_path, std::ios_base::app);
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    //warm up
    Triple default_tp("", "", "", dict);
    HDT * hdt = controller->get_snapshot_manager()->get_snapshot(0);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(hdt, default_tp, 0);
    int count = 0;
    while (snapshot_it->hasNext() && count < 100) {
        snapshot_it->next();
        //cout << "Result Warmup: " << triple->getSubject() << ", " << triple->getPredicate() << ", " << triple->getObject() << endl;
        count++;
    }
    PatchTreeIterator it = controller->get_patch_tree_manager()->get_patch_tree(1, dict)->iterator(&default_tp);
    PatchTreeKey key;
    PatchTreeValue val;
    count = 0;
    while (it.next(&key, &val) && count < 100) {
        //cout << "Result Warmup: " << triple->getSubject() << ", " << triple->getPredicate() << ", " << triple->getObject() << endl;
        count++;
    }
    //warm up finished

    Triple triple_pattern(s, p, o, dict);
    performance_file << "---PATTERN START: " << triple_pattern.to_string(*dict) << endl;

    performance_file << "--- ---VERSION MATERIALIZED" << endl;
    performance_file << "patch,offset,limit,count-ms,lookup-mus,results" << endl;
    for(int i = 0; i < patch_count; i++) {
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
    for(int i = 0; i < patch_count; i++) {
        for(int j = i + 1; j < patch_count; j++) {
            int result_count1 = 0;
            long dcount = measure_count_delta_materialized(triple_pattern, i, j, replications);
            long d1 = measure_lookup_delta_materialized(*dict, triple_pattern, offset, i, j, limit, replications, result_count1);
            performance_file << "" << i << "," << j << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
        }
    }

    performance_file << "--- ---VERSION" << endl;
    performance_file << "offset,limit,count-ms,lookup-mus,results" << endl;
    int result_count1 = 0;
    long dcount = measure_count_version(triple_pattern, replications);
    long d1 = measure_lookup_version(*dict, triple_pattern, offset, limit, replications, result_count1);
    performance_file << "" << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
}

void Evaluator::cleanup_controller() {
    //Controller::cleanup(controller);
    delete controller;
}

//
// Created by thibault on 13/03/18.
//

#ifndef THESIS_V5_EXPERIMENTER_H
#define THESIS_V5_EXPERIMENTER_H


#include "../controller/controller.h"

#define BASEURI "<http://example.org>"

const char k_path_separator =
#ifdef _WIN32
        '\\';
#else
        '/';
#endif
class Experimenter {
private:
    Controller* controller;
    string patchesBasePatch;
    string basePath;
    IteratorTripleString* get_from_file(string file);
    ofstream performance_file;

    std::ifstream::pos_type filesize(string file);

public:
    /*
     * constructor
     */
    Experimenter(string performance_filename, string basePath, string patchesBasePatch, ProgressListener *progressListener);
    std::ifstream::pos_type patchstore_size(Controller* controller);
    std::ifstream::pos_type temp_patchstore_size(Controller* controller);

    virtual ~Experimenter();
    PatchElementIteratorCombined* load_patch(int patch_id, DictionaryManager *dict, ProgressListener *progressListener);
    PatchElementIteratorCombined* reverse_load_patch(int patch_id, DictionaryManager *dict, ProgressListener *progressListener);

    /*
     * populates controller with patches starting from start_patch_tree_id upto end_id
     * if start_patch_tree_id > end_id, patches are added in reverse tree
     * if copy is true, copy patch tree files after patch with copy_id is inserted
     * if copy_id is not set, copy_id is (end_id - start_patch_tree_id)/2
     * copy is only meaningful in the forward patch tree case
     */
    void populate_patch_tree(int start_patch_tree_id, int end_id, ProgressListener *progressListener, bool copy, int copy_id,
                                 Controller controller);
    /*
 * populates controller with snapshots starting from start_patch_tree_id upto end_id
 */
    void
    populate_snapshot(int start_patch_tree_id, int end_id, ProgressListener *progressListener, Controller controller);
    Controller *getController() const;

    void print_store_structure();

    void write_performance_to_file();

    long test_lookup_version_materialized(Controller *controller, Triple triple_pattern, int offset, int patch_id, int limit);

    long test_lookup_delta_materialized(Controller *controller, Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit);

    long test_lookup_version(Controller *controller, Triple triple_pattern, int offset, int limit);

    void reverse(int start_snapshot, int end_snapshot, ProgressListener *progressListener);

    void reverse_opt(int start_snapshot, int end_snapshot, ProgressListener *progressListener);

    long long int
    measure_lookup_version_materialized(Dictionary &dict, Triple triple_pattern, int offset, int patch_id, int limit,
                                        int replications, int &result_count);

    long long int measure_count_version_materialized(Triple triple_pattern, int patch_id, int replications);

    long long int
    measure_count_delta_materialized(Triple triple_pattern, int patch_id_start, int patch_id_end, int replications);

    long long int measure_lookup_version(Dictionary &dict, Triple triple_pattern, int offset, int limit, int replications,
                                         int &result_count);

    long long int measure_count_version(Triple triple_pattern, int replications);

    void test_lookup(int patch_count, string s, string p, string o, int replications, int offset, int limit);

    long long int measure_lookup_delta_materialized(Dictionary &dict, Triple triple_pattern, int offset, int patch_id_start,
                                                    int patch_id_end, int limit, int replications, int &result_count);

    void extract_changeset(Controller controller, int patchtree_id, string path_to_files);
    void remove_forward_delta_chain(Controller controller, string basePath, int snapshot_id);

};


#endif //THESIS_V5_EXPERIMENTER_H

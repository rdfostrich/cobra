#include <util/StopWatch.hpp>
#include "controller.h"
#include "snapshot_patch_iterator_triple_id.h"
#include "patch_builder_streaming.h"
#include "../snapshot/combined_triple_iterator.h"
#include "../simpleprogresslistener.h"
#include <sys/stat.h>
#include <boost/filesystem.hpp>

Controller::Controller(string basePath, int8_t kc_opts, bool readonly) : patchTreeManager(new PatchTreeManager(basePath, kc_opts, readonly)), snapshotManager(new SnapshotManager(basePath, readonly)) {
    struct stat sb;
    if (!(stat(basePath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))) {
        throw std::invalid_argument("The provided path '" + basePath + "' is not a valid directory.");
    }
}

Controller::~Controller() {
    delete patchTreeManager;
    delete snapshotManager;
}


size_t Controller::get_version_materialized_count_estimated(const Triple& triple_pattern, int patch_id) const {
    return get_version_materialized_count(triple_pattern, patch_id, true).first;
}

std::pair<size_t, ResultEstimationType> Controller::get_version_materialized_count(const Triple& triple_pattern, int patch_id, bool allowEstimates) const {
    int snapshot_id = get_corresponding_snapshot_id(patch_id);
    if(snapshot_id < 0) {
        return std::make_pair(0, EXACT);
    }

    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
    size_t snapshot_count = snapshot_it->estimatedNumResults();
    if (!allowEstimates && snapshot_it->numResultEstimation() != EXACT) {
        snapshot_count = 0;
        while (snapshot_it->hasNext()) {
            snapshot_it->next();
            snapshot_count++;
        }
    }
    if(snapshot_id == patch_id) {
        return std::make_pair(snapshot_count, snapshot_it->numResultEstimation());
    }

    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    int patch_tree_id = get_patch_tree_id(patch_id);
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id, dict);
    if(patchTree == NULL) {
        return std::make_pair(snapshot_count, snapshot_it->numResultEstimation());
    }

    std::pair<PatchPosition, Triple> deletion_count_data = patchTree->deletion_count(triple_pattern, patch_id);
    size_t addition_count = patchTree->addition_count(patch_id, triple_pattern);
    return std::make_pair(snapshot_count - deletion_count_data.first + addition_count, snapshot_it->numResultEstimation());
}

TripleIterator* Controller::get_version_materialized(const Triple &triple_pattern, int offset, int patch_id) const {
    // Find the snapshot
    int snapshot_id = get_corresponding_snapshot_id(patch_id);
    if(snapshot_id < 0) {
        //throw std::invalid_argument("No snapshot was found for version " + std::to_string(patch_id));
        return new EmptyTripleIterator();
    }
    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);

    // Simple case: We are requesting a snapshot, delegate lookup to that snapshot.
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
    if(snapshot_id == patch_id) {
        return new SnapshotTripleIterator(snapshot_it);
    }
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    // Otherwise, we have to prepare an iterator for a certain patch
    int patch_tree_id = get_patch_tree_id(patch_id);
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id, dict);
    if(patchTree == NULL) {
        return new SnapshotTripleIterator(snapshot_it);
    }
    PositionedTripleIterator* deletion_it = NULL;
    long added_offset = 0;
    bool check_offseted_deletions = true;

    // Limit the patch id to the latest available patch id
//    int max_patch_id = patchTree->get_max_patch_id();
//    if (patch_id > max_patch_id) {
//        patch_id = max_patch_id;
//    }

    std::pair<PatchPosition, Triple> deletion_count_data = patchTree->deletion_count(triple_pattern, patch_id);
    // This loop continuously determines new snapshot iterators until it finds one that contains
    // no new deletions with respect to the snapshot iterator from last iteration.
    // This loop is required to handle special cases like the one in the ControllerTest::EdgeCase1.
    // As worst-case, this loop will take O(n) (n:dataset size), as an optimization we can look
    // into storing long consecutive chains of deletions more efficiently.
    while(check_offseted_deletions) {
        if (snapshot_it->hasNext()) { // We have elements left in the snapshot we should apply deletions to
            // Determine the first triple in the original snapshot and use it as offset for the deletion iterator
            TripleID *tripleId = snapshot_it->next();
            Triple firstTriple(tripleId->getSubject(), tripleId->getPredicate(), tripleId->getObject());
            deletion_it = patchTree->deletion_iterator_from(firstTriple, patch_id, triple_pattern);
            deletion_it->getPatchTreeIterator()->set_early_break(true);

            // Calculate a new offset, taking into account deletions.
            PositionedTriple first_deletion_triple;
            long snapshot_offset = 0;
            if (deletion_it->next(&first_deletion_triple, true)) {
                snapshot_offset = first_deletion_triple.position;
            } else {
                // The exact snapshot triple could not be found as a deletion
                if (patchTree->get_spo_comparator()->compare(firstTriple, deletion_count_data.second) < 0) {
                    // If the snapshot triple is smaller than the largest deletion,
                    // set the offset to zero, as all deletions will come *after* this triple.

                    // Note that it should impossible that there would exist a deletion *before* this snapshot triple,
                    // otherwise we would already have found this triple as a snapshot triple before.
                    // If we would run into issues because of this after all, we could do a backwards step with
                    // deletion_it and see if we find a triple matching the pattern, and use its position.

                    snapshot_offset = 0;
                } else {
                    // If the snapshot triple is larger than the largest deletion,
                    // set the offset to the total number of deletions.
                    snapshot_offset = deletion_count_data.first;
                }
            }
            long previous_added_offset = added_offset;
            added_offset = snapshot_offset;

            // Make a new snapshot iterator for the new offset
            // TODO: look into reusing the snapshot iterator and applying a relative offset (NOTE: I tried it before, it's trickier than it seems...)
            delete snapshot_it;
            snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset + added_offset);

            // Check if we need to loop again
            check_offseted_deletions = previous_added_offset < added_offset;
            if(check_offseted_deletions) {
                delete deletion_it;
                deletion_it = NULL;
            }
        } else {
            check_offseted_deletions = false;
        }
    }
    return new SnapshotPatchIteratorTripleID(snapshot_it, deletion_it, patchTree->get_spo_comparator(), snapshot, triple_pattern, patchTree, patch_id, offset, deletion_count_data.first);
}

std::pair<size_t, ResultEstimationType> Controller::get_delta_materialized_count(const Triple &triple_pattern, int patch_id_start, int patch_id_end, bool allowEstimates) const {
    int snapshot_id_start = get_corresponding_snapshot_id(patch_id_start);
    int snapshot_id_end = get_corresponding_snapshot_id(patch_id_end);
    int patch_tree_id_end = get_patch_tree_id(patch_id_end);
    int patch_tree_id_start = get_patch_tree_id(patch_id_start);
    // S_start <- P <- P_end
    if(snapshot_id_start == patch_id_start){
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_start);
        PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id_end, dict);
        size_t count = patchTree->deletion_count(triple_pattern, patch_id_end).first + patchTree->addition_count(patch_id_end, triple_pattern);
        return std::make_pair(count, EXACT);
    }
    //P_start -> P -> S_end
    else if(snapshot_id_end == patch_id_end){
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_end);
        PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id_start, dict);
        size_t count = patchTree->deletion_count(triple_pattern, patch_id_start).first + patchTree->addition_count(patch_id_start, triple_pattern);
        return std::make_pair(count, EXACT);
    }
    // reverse tree and forward tree case P_start -> S <- P_end
    else if(snapshot_id_start > patch_id_start && snapshot_id_end < patch_id_end) {
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_end);
        PatchTree* patchTreeForward = get_patch_tree_manager()->get_patch_tree(patch_tree_id_end, dict);
        PatchTree* patchTreeReverse = get_patch_tree_manager()->get_patch_tree(patch_tree_id_start, dict);
        size_t count = patchTreeForward->deletion_count(triple_pattern, patch_id_end).first + patchTreeForward->addition_count(patch_id_end, triple_pattern) + patchTreeReverse->deletion_count(triple_pattern, patch_id_start).first + patchTreeReverse->addition_count(patch_id_start, triple_pattern);
        return std::make_pair(count, UP_TO);
    }
    else{
        if (allowEstimates) {
            DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_start);
            PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id_start, dict);
            size_t count_start = patchTree->deletion_count(triple_pattern, patch_id_start).first + patchTree->addition_count(patch_id_start, triple_pattern);
            size_t count_end = patchTree->deletion_count(triple_pattern, patch_id_end).first + patchTree->addition_count(patch_id_end, triple_pattern);
            // There may be an overlap between the delta-triples from start and end.
            // This overlap is not easy to determine, so we ignore it when possible.
            // The real count will never be higher this value, because we should subtract the overlap count.
            return std::make_pair(count_start + count_end, UP_TO);
        } else {
            return std::make_pair(get_delta_materialized(triple_pattern, 0, patch_id_start, patch_id_end)->get_count(), EXACT);
        }
    }
}

size_t Controller::get_delta_materialized_count_estimated(const Triple &triple_pattern, int patch_id_start, int patch_id_end) const {
    return get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true).second;
}

TripleDeltaIterator* Controller::get_delta_materialized(const Triple &triple_pattern, int offset, int patch_id_start,
                                                        int patch_id_end) const {
    if (patch_id_end <= patch_id_start) {
        return new EmptyTripleDeltaIterator();
    }

    // Find the snapshot
    int snapshot_id_start = get_corresponding_snapshot_id(patch_id_start);
    int snapshot_id_end = get_corresponding_snapshot_id(patch_id_end);
    if (snapshot_id_start < 0 || snapshot_id_end < 0) {
        return new EmptyTripleDeltaIterator();
    }

    // start = snapshot, end = snapshot
    if(snapshot_id_start == patch_id_start && snapshot_id_end == patch_id_end) {
        // TODO: implement this when multiple snapshots are supported
        throw std::invalid_argument("Multiple snapshots are not supported.");
    }

    // start = snapshot, end = patch
    if(snapshot_id_start == patch_id_start && snapshot_id_end != patch_id_end) {
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_end);
        if (snapshot_id_start == snapshot_id_end) {
            // Return iterator for the end patch relative to the start snapshot
            int patch_tree_id = get_patch_tree_id(patch_id_end);
            PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id, dict);
            if(patchTree == NULL) {
                throw std::invalid_argument("Could not find the given end patch id");
            }
            if (TripleStore::is_default_tree(triple_pattern)) {
                return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_end))->offset(offset);
            } else {
                return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_end))->offset(offset);
            }
        } else {
            // TODO: implement this when multiple snapshots are supported
            throw std::invalid_argument("Multiple snapshots are not supported.");
        }
    }

    // start = patch, end = snapshot
    if(snapshot_id_start != patch_id_start && snapshot_id_end == patch_id_end) {
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_end);
        if (snapshot_id_start == snapshot_id_end) {
            // Return iterator for the end patch relative to the start snapshot
            int patch_tree_id = get_patch_tree_id(patch_id_start);
            PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id, dict);
            if(patchTree == NULL) {
                throw std::invalid_argument("Could not find the given end patch id");
            }
            if (TripleStore::is_default_tree(triple_pattern)) {
                return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_start))->offset(offset);
            } else {
                return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_start))->offset(offset);
            }
        } else {
            // TODO: implement this when multiple snapshots are supported
            throw std::invalid_argument("Multiple snapshots are not supported.");
        }

//        // TODO: implement this when multiple snapshots are supported
//        throw std::invalid_argument("Multiple snapshots are not supported.");
    }

    // start = patch, end = patch
    if(snapshot_id_start != patch_id_start && snapshot_id_end != patch_id_end) {
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_end);
        if (snapshot_id_start == snapshot_id_end) {

            // Return diff between two patches relative to the same snapshot
            int patch_tree_id_end = get_patch_tree_id(patch_id_end);
            int patch_tree_id_start = get_patch_tree_id(patch_id_start);

            // forward patch tree (same tree) S <- P_start <- P_end
            if(snapshot_id_start < patch_tree_id_start && patch_tree_id_end == patch_tree_id_start){

                PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id_end, dict);
                if(patchTree == NULL) {
                    throw std::invalid_argument("Could not find the given end patch id");
                }

                if (TripleStore::is_default_tree(triple_pattern)) {
                    return (new ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_start, patch_id_end))->offset(offset);
                } else {
                    return (new ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_start, patch_id_end))->offset(offset);
                }
            }
            //reverse patch tree (same tree) P_start -> P_end -> S
            else if(snapshot_id_start > patch_tree_id_start && patch_tree_id_end == patch_tree_id_start){

                PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id_end, dict);
                if(patchTree == NULL) {
                    throw std::invalid_argument("Could not find the given end patch id");
                }

                if (TripleStore::is_default_tree(triple_pattern)) {
                    return (new ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_start, patch_id_end, true))->offset(offset);
                } else {
                    return (new ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_start, patch_id_end, true))->offset(offset);
                }
            }
            // reverse tree and forward tree case P_start -> S <- P_end
            else{
//                int patch_tree_id = get_patch_tree_id(patch_id_start);
//                PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id, dict);
//                if(patchTree == NULL) {
//                    throw std::invalid_argument("Could not find the given end patch id");
//                }
//                if (TripleStore::is_default_tree(triple_pattern)) {
//                    return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_start))->offset(offset);
//                } else {
//                    return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_start))->offset(offset);
//                }
//
//                int patch_tree_id = get_patch_tree_id(patch_id_end);
//                PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_tree_id, dict);
//                if(patchTree == NULL) {
//                    throw std::invalid_argument("Could not find the given end patch id");
//                }
//                if (TripleStore::is_default_tree(triple_pattern)) {
//                    return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_end))->offset(offset);
//                } else {
//                    return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_end))->offset(offset);
//                }
                PatchTree* patchTreeReverse = get_patch_tree_manager()->get_patch_tree(patch_tree_id_start, dict);
                PatchTree* patchTreeForward = get_patch_tree_manager()->get_patch_tree(patch_tree_id_end, dict);
                if(patchTreeReverse == NULL || patchTreeForward == NULL) {
                    throw std::invalid_argument("Could not find the given patch id");
                }

                if (TripleStore::is_default_tree(triple_pattern)) {
                    return (new BiDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>(
                            new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTreeReverse,
                                                                                            triple_pattern,
                                                                                            patch_id_start
                                                                                            ),
                            new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTreeForward,
                                                                                        triple_pattern,
                                                                                        patch_id_end),
                            patchTreeForward->get_spo_comparator()))->offset(offset);
                } else {
                    return (new BiDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(
                            new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTreeReverse,
                                                                                               triple_pattern,
                                                                                               patch_id_start),
                            new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTreeForward,
                                                                                               triple_pattern,
                                                                                               patch_id_end),
                            patchTreeForward->get_spo_comparator()))->offset(offset);
                }

            }

        } else {
            // TODO: implement this when multiple snapshots are supported
            throw std::invalid_argument("Multiple snapshots are not supported.");
        }
    }
    return nullptr;
}

std::pair<size_t, ResultEstimationType> Controller::get_version_count(const Triple &triple_pattern, bool allowEstimates) const {
    // TODO: this will require some changes when we support multiple snapshots.
    // Find the snapshot an count its elements
    HDT* snapshot = get_snapshot_manager()->get_snapshot(0);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
    size_t count = snapshot_it->estimatedNumResults();
    if (!allowEstimates && snapshot_it->numResultEstimation() != EXACT) {
        count = 0;
        while (snapshot_it->hasNext()) {
            snapshot_it->next();
            count++;
        }
    }

    // Count the additions for all versions
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(0);
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(0, dict);
    if (patchTree != NULL) {
        count += patchTree->addition_count(0, triple_pattern);
    }
    return std::make_pair(count, allowEstimates ? snapshot_it->numResultEstimation() : EXACT);
}

std::pair<size_t, ResultEstimationType> Controller::get_partial_version_count(const Triple &triple_pattern, bool allowEstimates) const {
    // TODO: this will require some changes when we support multiple snapshots.
    int snapshot_id = snapshotManager->get_snapshots().begin()->first;
    int reverse_patch_tree_id = snapshot_id - 1;
    int forward_patch_tree_id = snapshot_id + 1;

    // Find the snapshot an count its elements
    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
    PatchTree* reverse_patch_tree = get_patch_tree_manager()->get_patch_tree(reverse_patch_tree_id, dict); // Can be null
    PatchTree* forward_patch_tree = get_patch_tree_manager()->get_patch_tree(forward_patch_tree_id, dict); // Can be null

    size_t count = snapshot_it->estimatedNumResults();
    if (!allowEstimates && snapshot_it->numResultEstimation() != EXACT) {
        count = 0;
        while (snapshot_it->hasNext()) {
            snapshot_it->next();
            count++;
        }
    }

    // Count the additions for all versions
    if (reverse_patch_tree != NULL) {
        count += reverse_patch_tree->addition_count(-1, triple_pattern);
    }
    if (forward_patch_tree != NULL) {
        count += forward_patch_tree->addition_count(-1, triple_pattern);
    }

    return std::make_pair(count, UP_TO);
}

size_t Controller::get_version_count_estimated(const Triple &triple_pattern) const {
    return get_version_count(triple_pattern, true).first;
}
TripleVersionsIterator* Controller::get_partial_version(const Triple &triple_pattern, int offset) const {
    int snapshot_id = snapshotManager->get_snapshots().begin()->first;
    int reverse_patch_tree_id = snapshot_id - 1;
    int forward_patch_tree_id = snapshot_id + 1;

    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    PatchTree* reverse_patch_tree = get_patch_tree_manager()->get_patch_tree(reverse_patch_tree_id, dict); // Can be null
    PatchTree* forward_patch_tree = get_patch_tree_manager()->get_patch_tree(forward_patch_tree_id, dict); // Can be null

    // Snapshots have already been offsetted, calculate the remaining offset.
    // After this, offset will only be >0 if we are past the snapshot elements and at the additions.
    if (snapshot_it->numResultEstimation() == EXACT) {
        offset -= snapshot_it->estimatedNumResults();
        if (offset <= 0) {
            offset = 0;
        } else {
            delete snapshot_it;
            snapshot_it = NULL;
        }
    } else {
        IteratorTripleID *tmp_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
        while (tmp_it->hasNext() && offset > 0) {
            tmp_it->next();
            offset--;
        }
        delete tmp_it;
    }
    return (new TripleVersionsIterator(triple_pattern, snapshot_it, reverse_patch_tree, forward_patch_tree, snapshot_id))->offset(offset);
}
TripleVersionsIterator* Controller::get_version(const Triple &triple_pattern, int offset) const {
//    // TODO: this will require some changes when we support multiple snapshots. (probably just a simple merge for all snapshots with what is already here)
//    // Find the snapshot
//    int snapshot_id = 0;
//    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
//    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
//    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
//    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(snapshot_id, dict); // Can be null, if only snapshot is available
//
//    // Snapshots have already been offsetted, calculate the remaining offset.
//    // After this, offset will only be >0 if we are past the snapshot elements and at the additions.
//    if (snapshot_it->numResultEstimation() == EXACT) {
//        offset -= snapshot_it->estimatedNumResults();
//        if (offset <= 0) {
//            offset = 0;
//        } else {
//            delete snapshot_it;
//            snapshot_it = NULL;
//        }
//    } else {
//        IteratorTripleID *tmp_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
//        while (tmp_it->hasNext() && offset > 0) {
//            tmp_it->next();
//            offset--;
//        }
//        delete tmp_it;
//    }
//
//    return (new TripleVersionsIterator(triple_pattern, snapshot_it, patchTree, 0))->offset(offset);
    return NULL;
}

bool Controller::append(PatchElementIterator* patch_it, int patch_id, DictionaryManager* dict, bool check_uniqueness, ProgressListener* progressListener) {

    //find largest key smaller or equal to patch_id, this is the patch_tree_id
    auto it = patchTreeManager->get_patch_trees().lower_bound(patch_id); //gives elements equal to or greater than patch_id
    if(it == patchTreeManager->get_patch_trees().end() || it->first > patch_id) {
        if(it == patchTreeManager->get_patch_trees().begin()) {
            // todo error no smaller element found
            return -1;
        }
        it--;
    }
    int patch_tree_id = it->first;
    return get_patch_tree_manager()->append(patch_it, patch_id, patch_tree_id, check_uniqueness, progressListener, dict);
}

bool Controller::reverse_append(PatchElementIterator* patch_it, int patch_id, DictionaryManager* dict, bool check_uniqueness, ProgressListener* progressListener) {

    //find smallest element larger than or equal to patch_id, this is the patch_tree_id
    int patch_tree_id;
    if(patchTreeManager->get_patch_trees().find(patch_id) == patchTreeManager->get_patch_trees().end()){
        auto iterator = patchTreeManager->get_patch_trees().upper_bound(patch_id); //returns first element bigger than patch_id
        patch_tree_id = iterator->first;
    }
    else{
        patch_tree_id = patch_id;
    }
    return get_patch_tree_manager()->reverse_append(patch_it, patch_id, patch_tree_id, dict, check_uniqueness, progressListener);
}


PatchTreeManager* Controller::get_patch_tree_manager() const {
    return patchTreeManager;
}

SnapshotManager* Controller::get_snapshot_manager() const {
    return snapshotManager;
}

DictionaryManager *Controller::get_dictionary_manager(int patch_id) const {
    int snapshot_id = get_corresponding_snapshot_id(patch_id);
    if(snapshot_id < 0) {
        throw std::invalid_argument("No snapshot has been created yet.");
    }
    get_snapshot_manager()->get_snapshot(snapshot_id); // Force a snapshot load
    return get_snapshot_manager()->get_dictionary_manager(snapshot_id);
}

int Controller::get_max_patch_id() {
    get_snapshot_manager()->get_snapshot(0); // Make sure our first snapshot is loaded, otherwise KC might get intro trouble while reorganising since it needs the dict for that.
    int max_patch_id = get_patch_tree_manager()->get_max_patch_id(get_snapshot_manager()->get_dictionary_manager(0));
    if (max_patch_id < 0) {
        return get_corresponding_snapshot_id(0);
    }
    return max_patch_id;
}

void Controller::cleanup(string basePath, Controller* controller) {
    // Delete patch files
    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    std::list<int> patchMetadataToDelete;
    while(itP != patches.end()) {
        int id = itP->first;
        std::remove((basePath + PATCHTREE_FILENAME(id, "spo_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pos_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pso_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "sop_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "osp_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "spo_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pos_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pso_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "sop_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "osp_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "count_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "count_additions.tmp")).c_str());
        patchMetadataToDelete.push_back(id);
        itP++;
    }

    // Delete snapshot files
    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    std::map<int, HDT*>::iterator itS = snapshots.begin();
    std::list<int> patchDictsToDelete;
    while(itS != snapshots.end()) {
        int id = itS->first;
        std::remove((basePath + SNAPSHOT_FILENAME_BASE(id)).c_str());
        std::remove((basePath + SNAPSHOT_FILENAME_BASE(id) + ".index").c_str());

        patchDictsToDelete.push_back(id);
        itS++;
    }

    delete controller;

    // Delete dictionaries
    std::list<int>::iterator it1;
    for(it1=patchDictsToDelete.begin(); it1!=patchDictsToDelete.end(); ++it1) {
        DictionaryManager::cleanup(basePath, *it1);
    }

    // Delete metadata files
    std::list<int>::iterator it2;
    for(it2=patchMetadataToDelete.begin(); it2!=patchMetadataToDelete.end(); ++it2) {
        std::remove((basePath + METADATA_FILENAME_BASE(*it2)).c_str());
    }
}

PatchBuilder* Controller::new_patch_bulk() {
    return new PatchBuilder(this);
}

PatchBuilderStreaming *Controller::new_patch_stream() {
    return new PatchBuilderStreaming(this);
}

bool Controller::replace_patch_tree(string basePath, int patch_tree_id) {

    // remove old files
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions.tmp")).c_str());
    std::remove((basePath + METADATA_FILENAME_BASE(patch_tree_id)).c_str());

    //rename temp files
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "spo_deletions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_deletions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pos_deletions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_deletions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pso_deletions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_deletions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "sop_deletions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_deletions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "osp_deletions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_deletions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "spo_additions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_additions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pos_additions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_additions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pso_additions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_additions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "sop_additions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_additions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "osp_additions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_additions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "count_additions")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions")).c_str());
    std::rename((basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "count_additions.tmp")).c_str(), (basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions.tmp")).c_str());
    std::rename((basePath + TEMP_METADATA_FILENAME_BASE(patch_tree_id)).c_str(), (basePath + METADATA_FILENAME_BASE(patch_tree_id)).c_str());
    return true;
}

int Controller::get_patch_tree_id(int patch_id)const {
    // case 1: S <- P <- P
    // case 2: S <- P <- P S <- P
    // case 3: S <- P <- P P -> P -> S <- P
    // case 4: P -> P -> S <- P <- P

    // return lower tree if reverse tree does not exists

    // return upper tree if reverse tree does exists
    std::map<int, HDT*> loaded_snapshots = get_snapshot_manager()->get_snapshots();

    std::map<int, HDT*>::iterator low, prev;
    low = loaded_snapshots.lower_bound(patch_id);
    int low_snapshot_id = -1;
    int high_snapshot_id = -1;

    if(low == loaded_snapshots.begin() && low == loaded_snapshots.end()) {
        // empty map
        return -1;
    }
    if (low == loaded_snapshots.end()) {
        low--;
        low_snapshot_id = low->first;
    } else if (low == loaded_snapshots.begin()) {
        low_snapshot_id = low->first;
    } else {
        prev = std::prev(low);
        high_snapshot_id = low->first;
        low_snapshot_id = prev->first;
    }

    // case 4
    if(low_snapshot_id > patch_id){
        return low_snapshot_id - 1;
    }

    std::map<int, PatchTree*> patches = get_patch_tree_manager()->get_patch_trees();
    int low_patch_tree_id = low_snapshot_id + 1 ; // todo make function of forward patch_tree_id
    if(high_snapshot_id >= 0){ // if high snapshot exists
        auto it = patches.find(high_snapshot_id - 1); // todo make function of reverse patch_tree_id
        if(it != patches.end()){ // if reverse patch tree exists
            int dist_to_low = patch_id - low_snapshot_id;
            int dist_to_high = high_snapshot_id -  patch_id;

            if(dist_to_high < dist_to_low){ // closer to reverse patch tree
                return high_snapshot_id - 1; // todo make function of reverse patch_tree_id
            }
        }
    }
    // check if forward chain exists
    auto it = patches.find(low_patch_tree_id); // todo make function of reverse patch_tree_id
    if(it == patches.end()){
        return -1;
    }
    return low_patch_tree_id;
}

int Controller::get_corresponding_snapshot_id(int patch_id) const {
    // get snapshot before patch_id
    // get snapshot after patch_id

    // if snapshot after patch_id does not exist or snapshot_after does not have reverse tree, return snapshot before patch_id
    // if reverse does exist, return snapshot id closest to patch_id (in case of tie, return lowest snapshot_id
    std::map<int, HDT*> loaded_snapshots = get_snapshot_manager()->get_snapshots();

    std::map<int, HDT*>::iterator low, prev;
    low = loaded_snapshots.lower_bound(patch_id);
    int low_snapshot_id = -1;
    int high_snapshot_id = -1;

    if(low == loaded_snapshots.begin() && low == loaded_snapshots.end()) {
        // empty map
        return -1;
    }
    if (low == loaded_snapshots.end()) {
        low--;
        low_snapshot_id = low->first;
    } else if (low == loaded_snapshots.begin()) {
        low_snapshot_id = low->first;
    } else {
        prev = std::prev(low);
        high_snapshot_id = low->first;
        low_snapshot_id = prev->first;
    }
    if(high_snapshot_id == patch_id)
        return high_snapshot_id;
    else if(low_snapshot_id == patch_id)
        return low_snapshot_id;
    else{
        std::map<int, PatchTree*> loaded_patches = get_patch_tree_manager()->get_patch_trees();
        int low_patch_tree_id = low_snapshot_id + 1 ; // todo make function of forward patch_tree_id
        if(high_snapshot_id >= 0){ // if high snapshot exists
            auto it = loaded_patches.find(high_snapshot_id - 1); // todo make function of reverse patch_tree_id
            if(it != loaded_patches.end()){ // if reverse patch tree exists
                int dist_to_low = patch_id - low_snapshot_id;
                int dist_to_high = high_snapshot_id -  patch_id;
                if(dist_to_high < dist_to_low){ // closer to reverse patch tree
                    return high_snapshot_id;
                }
            }
        }
        return low_snapshot_id;}
}

bool Controller::copy_patch_tree_files(string basePath, int patch_tree_id) {
    try {
        boost::filesystem::path source, target;

        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_deletions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "spo_deletions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_deletions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pos_deletions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_deletions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pso_deletions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_deletions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "sop_deletions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_deletions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "osp_deletions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_additions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "spo_additions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_additions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pos_additions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_additions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "pso_additions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_additions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "sop_additions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_additions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "osp_additions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions");
        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "count_additions");
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
//        source = basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions.tmp");
//        target = basePath + TEMP_PATCHTREE_FILENAME(patch_tree_id, "count_additions.tmp");
//        if (boost::filesystem::exists(source))
//            boost::filesystem::copy_file(source, target);
        source = basePath + METADATA_FILENAME_BASE(patch_tree_id);
        target = basePath + TEMP_METADATA_FILENAME_BASE(patch_tree_id);
        if (boost::filesystem::exists(source))
            boost::filesystem::copy_file(source, target);
    } catch (const boost::filesystem::filesystem_error& e){
        return false;
    }
    return true;
}

void Controller::remove_forward_chain(std::string basePath, int temp_snapshot_id){

    int patch_tree_id = temp_snapshot_id + 1;
    get_patch_tree_manager()->remove_patch_tree(patch_tree_id);
    get_snapshot_manager()->remove_snapshot(temp_snapshot_id);
    std::remove((basePath + PATCHDICT_FILENAME_BASE(temp_snapshot_id)).c_str());
    std::remove((basePath + METADATA_FILENAME_BASE(patch_tree_id)).c_str());
    std::remove((basePath + SNAPSHOT_FILENAME_BASE(temp_snapshot_id)).c_str());
    std::remove((basePath + SNAPSHOT_FILENAME_BASE(temp_snapshot_id) + ".index").c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_deletions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "spo_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pos_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "pso_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "sop_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "osp_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions")).c_str());
    std::remove((basePath + PATCHTREE_FILENAME(patch_tree_id, "count_additions.tmp")).c_str());

}

void Controller::extract_changeset(int patch_tree_id, std::string path_to_files) {
    patchTreeManager->get_patch_tree(patch_tree_id, get_dictionary_manager(patch_tree_id))->getTripleStore()->extract_additions(get_dictionary_manager(patch_tree_id), path_to_files);
    patchTreeManager->get_patch_tree(patch_tree_id, get_dictionary_manager(patch_tree_id))->getTripleStore()->extract_deletions(get_dictionary_manager(patch_tree_id), path_to_files);
}

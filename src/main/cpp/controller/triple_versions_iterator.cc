#include "triple_versions_iterator.h"
#include <algorithm>
#include <numeric>

TripleVersions::TripleVersions() : triple(new Triple()), versions(new vector<int>()) {}

TripleVersions::TripleVersions(Triple* triple, std::vector<int>* versions) : triple(triple), versions(versions) {
}

TripleVersions::~TripleVersions() {
    delete triple;
    delete versions;
}

Triple* TripleVersions::get_triple() {
    return triple;
}

vector<int>* TripleVersions::get_versions() {
    return versions;
}

TripleVersionsIterator::TripleVersionsIterator(Triple triple_pattern, IteratorTripleID* snapshot_it, PatchTree* reverse_patch_tree, PatchTree* forward_patch_tree, int snapshot_id)
        : triple_pattern(triple_pattern), snapshot_it(snapshot_it), reverse_patch_tree(reverse_patch_tree), forward_patch_tree(forward_patch_tree), forward_addition_it(NULL), reverse_addition_it(NULL), snapshot_id(snapshot_id) {
}

TripleVersionsIterator::~TripleVersionsIterator() {
    if (snapshot_it != NULL) delete snapshot_it;
    if (reverse_addition_it != NULL) delete reverse_addition_it;
    if (forward_addition_it != NULL) delete forward_addition_it;
    delete forward_key;
    delete reverse_key;
    delete forward_value;
    delete reverse_value;
}

inline void TripleVersionsIterator::forwardEraseDeletedVersions(std::vector<int> *versions, Triple *currentTriple,
                                                                int initial_version) {
    if (forward_patch_tree == NULL) {
        // If we only have a snapshot, return a single version annotation.
        versions->clear();
        versions->push_back(initial_version);
    } else {
        PatchTreeDeletionValue* deletion = forward_patch_tree->get_deletion_value(*currentTriple);
        versions->clear();
        versions->resize(forward_patch_tree->get_max_patch_id() + 1 - initial_version);
        std::iota(versions->begin(), versions->end(), initial_version); // Fill up the vector with all versions from initial_version to max_patch_id
        if (deletion != NULL) {
            for (int v_del = 0; v_del < deletion->get_size(); v_del++) {
                PatchTreeDeletionValueElement deletion_element = deletion->get_patch(v_del);
                // Erase-remove idiom on sorted vector, and maintain order
                auto pr = std::equal_range(versions->begin(), versions->end(), deletion_element.get_patch_id());
                versions->erase(pr.first, pr.second);
            }
        }
    }
}

inline void TripleVersionsIterator::reverseEraseDeletedVersions(std::vector<int>* versions, Triple* currentTriple, int initial_version) {
    if (reverse_patch_tree == NULL) {
        // If we only have a snapshot, return a single version annotation.
        versions->clear();
        versions->push_back(initial_version);
    } else {
        PatchTreeDeletionValue* deletion = reverse_patch_tree->get_deletion_value(*currentTriple);
        versions->clear();
        versions->resize(initial_version - reverse_patch_tree->get_min_patch_id() + 1);
        std::iota(versions->begin(), versions->end(), reverse_patch_tree->get_min_patch_id()); // Fill up the vector with all versions from min_patch_id to snapshot_id
        if (deletion != NULL) {
            for (int v_del = 0; v_del < deletion->get_size(); v_del++) {
                PatchTreeDeletionValueElement deletion_element = deletion->get_patch(v_del);
                auto it = std::find(versions->begin(), versions->end(), deletion_element.get_patch_id());
                if(it != versions->end())
                    versions->erase(it);
            }
        }
    }
}
inline void TripleVersionsIterator::erase_deleted_versions(std::vector<int>* versions, Triple* currentTriple, int reverse_version_from, int forward_version_from) {
    if (reverse_patch_tree == NULL && forward_patch_tree == NULL) {
        // If we only have a snapshot, return a single version annotation.
        versions->clear();
        versions->push_back(reverse_version_from); // assert reverse_version_from == forward_version_from == snapshot_id
        return;
    }
    else if ((reverse_patch_tree == NULL && forward_patch_tree != NULL)  || (reverse_version_from == -1 && forward_version_from != -1)) {
        versions->clear();
        versions->resize(forward_patch_tree->get_max_patch_id() + 1 - forward_version_from);
        std::iota(versions->begin(), versions->end(), forward_version_from); // Fill up the vector with all versions from initial_version to max_patch_id
    }
    else if ((reverse_patch_tree != NULL && forward_patch_tree == NULL) || (reverse_version_from != -1 && forward_version_from == -1)) {
        versions->clear();
        versions->resize(reverse_version_from - reverse_patch_tree->get_min_patch_id() + 1);
        std::iota(versions->begin(), versions->end(), reverse_patch_tree->get_min_patch_id()); // Fill up the vector with all versions from min_patch_id to snapshot_id
    }
    else {
        versions->clear();

        versions->resize(forward_patch_tree->get_max_patch_id() + 1 - reverse_patch_tree->get_min_patch_id());
        std::iota(versions->begin(), versions->end(), reverse_patch_tree->get_min_patch_id()); // Fill up the vector with all versions from initial_version to max_patch_id
        if(reverse_version_from != forward_version_from){
            // erase middle section
            auto reverse_start = std::find(versions->begin(), versions->end(), reverse_version_from);
            auto forward_start = std::find(versions->begin(), versions->end(), forward_version_from);
            versions->erase(reverse_start + 1, forward_start);
        }
    }
    PatchTreeDeletionValue* reverse_deletion = NULL;
    if (reverse_patch_tree != NULL  && reverse_version_from != -1)
        reverse_deletion = reverse_patch_tree->get_deletion_value(*currentTriple);

    PatchTreeDeletionValue* forward_deletion = NULL;
    if (forward_patch_tree != NULL  && forward_version_from != -1)
        forward_deletion = forward_patch_tree->get_deletion_value(*currentTriple);

    if (forward_deletion != NULL && forward_version_from != -1) {
        for (int v_del = 0; v_del < forward_deletion->get_size(); v_del++) {
            PatchTreeDeletionValueElement deletion_element = forward_deletion->get_patch(v_del);
            auto it = std::find(versions->begin(), versions->end(), deletion_element.get_patch_id());
            if(it != versions->end())
                versions->erase(it);
        }
    }
    if (reverse_deletion != NULL && reverse_version_from != -1) {
        for (int v_del = 0; v_del < reverse_deletion->get_size(); v_del++) {
            PatchTreeDeletionValueElement deletion_element = reverse_deletion->get_patch(v_del);
            auto it = std::find(versions->begin(), versions->end(), deletion_element.get_patch_id());
            if(it != versions->end())
                versions->erase(it);
        }
    }
}

bool TripleVersionsIterator::next(TripleVersions* triple_versions) {
    // Loop over snapshot elements, and emit all versions minus the versions that have been deleted.
    if (snapshot_it != NULL && snapshot_it->hasNext()) {
        TripleID *tripleId = snapshot_it->next();
        Triple* currentTriple = triple_versions->get_triple();
        currentTriple->set_subject(tripleId->getSubject());
        currentTriple->set_predicate(tripleId->getPredicate());
        currentTriple->set_object(tripleId->getObject());
        erase_deleted_versions(triple_versions->get_versions(), currentTriple, snapshot_id, snapshot_id);
        return true;
    }

    // If we only have a snapshot, don't query the unavailable patch tree.
    if (reverse_patch_tree == NULL && forward_patch_tree == NULL) {
        return false;
    }

        // When we get here, no snapshot elements are left, so emit additions here
        if (forward_addition_it == NULL && forward_patch_tree != NULL){
            forward_addition_it = forward_patch_tree->addition_iterator(triple_pattern);
        }
        if (reverse_addition_it == NULL && reverse_patch_tree != NULL){
            reverse_addition_it = reverse_patch_tree->addition_iterator(triple_pattern);
        }
    comp = forward_addition_it->getComparator();

    // sort-merge join additions
    PatchTreeAdditionValue reverse_value;
    PatchTreeAdditionValue forward_value;
    int origin=0;
    while (origin != 2) {
        origin = join_next_addition(reverse_addition_it, forward_addition_it, &reverse_value, &forward_value);
        if(origin == 0) { //triple is present in both addition streams
            if (!forward_value.is_local_change(forward_value.get_patch_id_at(0))) {

                Triple* currentTriple = triple_versions->get_triple();
                currentTriple->set_subject(forward_key->get_subject());
                currentTriple->set_predicate(forward_key->get_predicate());
                currentTriple->set_object(forward_key->get_object());

                erase_deleted_versions(triple_versions->get_versions(), currentTriple,
                                       reverse_value.get_patch_id_at(reverse_value.get_size() - 1), forward_value.get_patch_id_at(0));
                return true;
            }
        }
        if(origin == -1) { //triple is present in reverse_addition stream
            if (!reverse_value.is_reverse_local_change(reverse_value.get_patch_id_at(0))) {

                Triple* currentTriple = triple_versions->get_triple();
                currentTriple->set_subject(reverse_key->get_subject());
                currentTriple->set_predicate(reverse_key->get_predicate());
                currentTriple->set_object(reverse_key->get_object());

                erase_deleted_versions(triple_versions->get_versions(), currentTriple,
                                       reverse_value.get_patch_id_at(reverse_value.get_size() - 1), -1);

                return true;
            }
        }
        else if (origin == 1){ //triple is present in forward_addition stream
            if (!forward_value.is_local_change(forward_value.get_patch_id_at(0))) {
                Triple* currentTriple = triple_versions->get_triple();
                currentTriple->set_subject(forward_key->get_subject());
                currentTriple->set_predicate(forward_key->get_predicate());
                currentTriple->set_object(forward_key->get_object());
                erase_deleted_versions(triple_versions->get_versions(), currentTriple,
                                       -1, forward_value.get_patch_id_at(0));
                return true;
            }
        }
    }
    return false;

}


size_t TripleVersionsIterator::get_count() {
    size_t count = 0;
    TripleVersions tv;
    while (next(&tv)) count++;
    return count;
}


TripleVersionsIterator* TripleVersionsIterator::offset(int offset) {
    TripleVersions tv;
    while(offset-- > 0 && next(&tv)); //TODO can be quicker if we just do next_addition
    return this;
}

int
TripleVersionsIterator::join_next_addition(PatchTreeIterator *reverse_additions, PatchTreeIterator *forward_additions,
                                           PatchTreeAdditionValue *arg_reverse_value, PatchTreeAdditionValue *arg_forward_value) {

    if(reverse_addition_it == NULL) {
        advance_reverse = false;
        valid_reverse = false;
    }
    while(valid_forward || valid_reverse){
        if(advance_reverse){
            valid_reverse = reverse_addition_it->next_addition(reverse_key, reverse_value);
            advance_reverse = false;
        }
        if(advance_forward){
            valid_forward = forward_addition_it->next_addition(forward_key, forward_value);
            advance_forward = false;
        }

        if(valid_forward && valid_reverse) {
            int comparison = comp->compare(*reverse_key, *forward_key);
            if (comparison < 0) {
                advance_reverse = true;
                *arg_reverse_value = *reverse_value;
                return -1;

            } else if (comparison > 0) {
                advance_forward = true;
                *arg_forward_value = *forward_value;
                return 1;
            } else {
                *arg_forward_value = *forward_value;
                *arg_reverse_value = *reverse_value;
                advance_reverse = true;
                advance_forward = true;
                return 0;
            }
        }
        else if (!valid_forward && valid_reverse) { // emit remaining additions from reverse
            advance_reverse = true;
            *arg_reverse_value = *reverse_value;
            return -1;
        }
        else if (valid_forward && !valid_reverse){ // emit remaining additions from forward
            advance_forward = true;
            *arg_forward_value = *forward_value;
            return 1;
        }
    }
    return 2;

}

#ifndef TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H
#define TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H

#include <vector>
#include "../patch/triple.h"
#include "../patch/patch_tree.h"

// Triple annotated with addition/deletion.
class TripleVersions {
protected:
    Triple* triple;
    std::vector<int>* versions;
public:
    TripleVersions();
    TripleVersions(Triple* triple, std::vector<int>* versions);
    ~TripleVersions();
    Triple* get_triple();
    std::vector<int>* get_versions();
};

class TripleVersionsIterator {
protected:
    int snapshot_id;
//    bool reverse;
    Triple triple_pattern;
    IteratorTripleID* snapshot_it;
    PatchTree* reverse_patch_tree;
    PatchTree* forward_patch_tree;
    PatchTreeIterator* reverse_addition_it;
    PatchTreeIterator* forward_addition_it;
    PatchTreeKeyComparator* comp;

    inline void forwardEraseDeletedVersions(std::vector<int> *versions, Triple *currentTriple, int initial_version);
    inline void reverseEraseDeletedVersions(vector<int> *versions, Triple *currentTriple, int initial_version);
    inline void erase_deleted_versions(vector<int> *versions, Triple *currentTriple, int reverse_version_from, int forward_version_from);
    bool next_addition(Triple *triple, PatchTreeAdditionValue &reverse_value, bool &valid_reverse,
                       PatchTreeAdditionValue &forward_value, bool &valid_forward);

    bool advance_forward = true, advance_reverse=true;
    bool valid_forward = true, valid_reverse=true;
    Triple *reverse_key = new Triple(), *forward_key = new Triple();
    PatchTreeAdditionValue *reverse_value = new PatchTreeAdditionValue(), *forward_value = new PatchTreeAdditionValue();
public:
    TripleVersionsIterator(Triple triple_pattern, IteratorTripleID* snapshot_it, PatchTree* reverse_patch_tree, PatchTree* forward_patch_tree, int snapshot_id);
    ~TripleVersionsIterator();
    bool next(TripleVersions* triple_versions);
    size_t get_count();
    TripleVersionsIterator* offset(int offset);

    /**
     *
     * @param reverse_additions
     * @param forward_additions
     * @param reverse_value
     * @param forward_value
     * @return 0 if triple was present in both streams, 1 if triple was present in forward, -1 in reverse
     */
    int join_next_addition(PatchTreeIterator *reverse_additions, PatchTreeIterator *forward_additions,
                           PatchTreeAdditionValue *reverse_value, PatchTreeAdditionValue *forward_value);
};


#endif //TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H

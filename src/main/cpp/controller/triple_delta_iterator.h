#ifndef TPFPATCH_STORE_TRIPLEDELTAITERATOR_H
#define TPFPATCH_STORE_TRIPLEDELTAITERATOR_H

#include "../patch/triple.h"
#include "../patch/patch_tree_iterator.h"
#include "../patch/patch_tree.h"

// Triple annotated with addition/deletion.
class TripleDelta {
protected:
    Triple* triple;
public:
    void setTriple(Triple *triple);

protected:
    bool addition;
public:
    TripleDelta();
    TripleDelta(Triple* triple, bool addition);
    ~TripleDelta();
    Triple* get_triple();
    bool is_addition();
    void set_addition(bool addition);

    bool isAddition() const;
};

// Iterator for triples annotated with addition/deletion.
class TripleDeltaIterator {
public:
    virtual ~TripleDeltaIterator() = 0;
    virtual bool next(TripleDelta* triple) = 0;
    size_t get_count();
    TripleDeltaIterator* offset(int offset);
};

// Triple delta iterator where elements from a single patch are emitted.
template <class DV>
class ForwardPatchTripleDeltaIterator : public TripleDeltaIterator {
protected:
    PatchTreeIteratorBase<DV>* it;
    PatchTreeValueBase<DV>* value;
public:
    ForwardPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_end);
    ~ForwardPatchTripleDeltaIterator();
    bool next(TripleDelta* triple);
};

// Triple delta iterator where elements only differences between two patches are emitted.
template <class DV>
class ForwardDiffPatchTripleDeltaIterator : public ForwardPatchTripleDeltaIterator<DV> {
protected:
    int patch_id_start;
    int patch_id_end;
    bool reverse;
public:
    ForwardDiffPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end, bool reverse = false);
    bool next(TripleDelta* triple);
};

class EmptyTripleDeltaIterator : public TripleDeltaIterator {
public:
    bool next(TripleDelta* triple);
};

template <class DV>
class BiDiffPatchTripleDeltaIterator : public TripleDeltaIterator {
protected:
//    PatchTree* patchTreeReverse;
//    PatchTree* patchTreeForward;
//    int patch_id_start;
//    int patch_id_end;
//    PatchTreeAdditionValue* reverse_add_value;
//    DV* reverse_del_value;
//    PatchTreeAdditionValue* forward_add_value;
//    DV* forward_del_value;
//    PatchTreeKey* forward_add_key;
//    PatchTreeKey* reverse_add_key;
//    PatchTreeKey* forward_del_key;
//    PatchTreeKey* reverse_del_key;
//
//    PatchTreeIteratorBase<DV>* reverse_iterator;
//    PatchTreeIteratorBase<DV>* forward_iterator;
//    bool reverse_next_deletion = true;
//    bool forward_next_deletion = true;
//    bool reverse_next_addition = true;
//    bool forward_next_addition = true;
//
//    bool advance_forward_add = true;
//    bool advance_reverse_add = true;
//    bool advance_reverse_del = true;
//    bool advance_forward_del = true;
    ForwardPatchTripleDeltaIterator<DV>* reverse_iterator;
    ForwardPatchTripleDeltaIterator<DV>* forward_iterator;
    TripleDelta* forward_triple_delta = new TripleDelta();
    TripleDelta* reverse_triple_delta = new TripleDelta();
    bool advance_forward = true;
    bool advance_reverse = true;
    bool valid_forward = true;
    bool valid_reverse = true;
    PatchTreeKeyComparator *pComparator;

public:
    BiDiffPatchTripleDeltaIterator(ForwardPatchTripleDeltaIterator<DV> *reverse_iterator,
                                       ForwardPatchTripleDeltaIterator<DV> *forward_iterator,
                                       PatchTreeKeyComparator *pComparator);
    bool next(TripleDelta* triple) override;
    ~BiDiffPatchTripleDeltaIterator();

};




#endif //TPFPATCH_STORE_TRIPLEDELTAITERATOR_H

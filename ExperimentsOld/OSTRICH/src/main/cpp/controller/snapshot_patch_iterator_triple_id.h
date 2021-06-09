#ifndef TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H
#define TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H

#include <Triples.hpp>
#include <Iterator.hpp>
#include <HDT.hpp>
#include "../patch/positioned_triple_iterator.h"
#include "../patch/patch_tree.h"

class SnapshotPatchIteratorTripleID : public TripleIterator {
private:
    IteratorTripleID* snapshot_it;
    PositionedTripleIterator* deletion_it;
    PatchTreeTripleIterator* addition_it;
    PatchTreeKeyComparator* spo_comparator;

    bool has_last_deleted_triple;
    PositionedTriple* last_deleted_triple;
    HDT* snapshot;
    const Triple triple_pattern;
    PatchTree* patchTree;
    int patch_id;
    int offset;
    PatchPosition deletion_count;
public:
    SnapshotPatchIteratorTripleID(IteratorTripleID* snapshot_it, PositionedTripleIterator* deletion_it,
                                  PatchTreeKeyComparator* spo_comparator, HDT* snapshot, const Triple& triple_pattern,
                                  PatchTree* patchTree, int patch_id, int offset, PatchPosition deletion_count);
    ~SnapshotPatchIteratorTripleID();
    bool next(Triple* triple);
};


#endif //TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H

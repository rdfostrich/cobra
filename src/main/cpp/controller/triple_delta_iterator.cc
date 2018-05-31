#include "triple_delta_iterator.h"

TripleDelta::TripleDelta() : triple(new Triple()), addition(true) {}

TripleDelta::TripleDelta(Triple* triple, bool addition) : triple(triple), addition(addition) {}

Triple* TripleDelta::get_triple() {
    return triple;
}

bool TripleDelta::is_addition() {
    return addition;
}

TripleDelta::~TripleDelta() {
    delete triple;
}

void TripleDelta::set_addition(bool addition) {
    this->addition = addition;
}

void TripleDelta::setTriple(Triple *triple) {
    TripleDelta::triple = triple;
}

bool TripleDelta::isAddition() const {
    return addition;
}

TripleDeltaIterator::~TripleDeltaIterator() {}

TripleDeltaIterator* TripleDeltaIterator::offset(int offset) {
    TripleDelta td;
    while(offset-- > 0 && next(&td));
    return this;
}

size_t TripleDeltaIterator::get_count() {
    size_t count = 0;
    TripleDelta td;
    while (next(&td)) count++;
    return count;
}

bool EmptyTripleDeltaIterator::next(TripleDelta *triple) {
    return false;
}

template <class DV>
ForwardPatchTripleDeltaIterator<DV>::ForwardPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_end) : it(patchTree->iterator<DV>(&triple_pattern)) {
    it->set_patch_filter(patch_id_end, true);
    it->set_filter_local_changes(true);
    it->set_early_break(true);
    value = new PatchTreeValueBase<DV>();
}

template <class DV>
ForwardPatchTripleDeltaIterator<DV>::~ForwardPatchTripleDeltaIterator() {
    delete it;
    delete value;
}

template <class DV>
bool ForwardPatchTripleDeltaIterator<DV>::next(TripleDelta* triple) {
    bool ret = it->next(triple->get_triple(), value);
    triple->set_addition(value->is_addition(it->get_patch_id_filter(), true));
    return ret;
}

template <class DV>
ForwardDiffPatchTripleDeltaIterator<DV>::ForwardDiffPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end, bool reverse)
        : ForwardPatchTripleDeltaIterator<DV>(patchTree, triple_pattern, patch_id_end), patch_id_start(patch_id_start), patch_id_end(patch_id_end), reverse(reverse) {
    this->it->reset_patch_filter();
    this->it->set_filter_local_changes(false);
}

template <class DV>
bool ForwardDiffPatchTripleDeltaIterator<DV>::next(TripleDelta *triple) {
    bool valid;
    while ((valid = this->it->next(triple->get_triple(), this->value))
                    && this->value->is_delta_type_equal(patch_id_start, patch_id_end)) {}
    if (valid) {
        if(reverse)
            triple->set_addition(!this->value->is_deletion(patch_id_end, true));
        else
            triple->set_addition(this->value->is_addition(patch_id_end, true));

    }
    return valid;
}

template<class DV>
BiDiffPatchTripleDeltaIterator<DV>::BiDiffPatchTripleDeltaIterator(ForwardPatchTripleDeltaIterator<DV>* reverse_iterator, ForwardPatchTripleDeltaIterator<DV>* forward_iterator, PatchTreeKeyComparator *pComparator): forward_iterator(forward_iterator), reverse_iterator(reverse_iterator), pComparator(pComparator) {}

template<class DV>
BiDiffPatchTripleDeltaIterator<DV>::~BiDiffPatchTripleDeltaIterator()
{
    delete forward_iterator;
    delete reverse_iterator;
//    delete forward_triple_delta;
//    delete reverse_triple_delta;
}

//bool IsBitSet(uint8_t num, int bit)
//{
//    return 1 == ( (num >> bit) & 1);
//}

template<class DV>
bool BiDiffPatchTripleDeltaIterator<DV>::next(TripleDelta *triple) {


    while (valid_forward || valid_reverse) {

        if(advance_forward){
            valid_forward = forward_iterator->next(forward_triple_delta);
            advance_forward = false;
        }
        if(advance_reverse){
            valid_reverse = reverse_iterator->next(reverse_triple_delta);
            advance_reverse = false;
        }

        if(valid_forward && valid_reverse){
            int comparison = pComparator->compare(*forward_triple_delta->get_triple(), *reverse_triple_delta->get_triple());
            if(comparison < 0){
                advance_forward = true;

                *triple = *forward_triple_delta;
                return true;

            }
            else if(comparison > 0){
                advance_reverse = true;
                *triple = *reverse_triple_delta;

                return true;
            }
            else {
                advance_reverse = true;
                advance_forward = true;
            }
        }

        else if(valid_forward && !valid_reverse){
            //emit remaining forward triples
            advance_forward = true;
            *triple = *forward_triple_delta;
            return true;
        }
        else if(!valid_forward && valid_reverse){
            //emit remaining reverse triples
            advance_reverse = true;
            *triple = *reverse_triple_delta;

            return true;
        }
    }
    return false;

//    while(forward_next_deletion || forward_next_addition || reverse_next_deletion || forward_next_deletion){
//        if(advance_forward_add){
//            forward_next_addition = forward_iterator->next_addition(forward_add_key, forward_add_value);
//            if(!forward_next_addition) {
//                forward_add_key->set_subject(dict->getMaxSubjectID());
//                forward_add_key->set_predicate(dict->getMaxPredicateID());
//                forward_add_key->set_object(dict->getMaxObjectID());
//            }
//            advance_forward_add = false;
//        }
//        if(advance_reverse_add){
//            reverse_next_addition = reverse_iterator->next_addition(reverse_add_key, reverse_add_value);
//            if(!reverse_next_addition) {
//                reverse_add_key->set_subject(dict->getMaxSubjectID());
//                reverse_add_key->set_predicate(dict->getMaxPredicateID());
//                reverse_add_key->set_object(dict->getMaxObjectID());
//            }
//            advance_reverse_add = false;
//        }
//        if(advance_forward_del){
//            forward_next_deletion = forward_iterator->next_deletion(forward_del_key, forward_del_value);
//            if(!forward_next_deletion) {
//                forward_del_key->set_subject(dict->getMaxSubjectID());
//                forward_del_key->set_predicate(dict->getMaxPredicateID());
//                forward_del_key->set_object(dict->getMaxObjectID());
//            }
//            advance_forward_del = false;
//        }
//        if(advance_reverse_del){
//            reverse_next_deletion = reverse_iterator->next_deletion(reverse_del_key, reverse_del_value);
//            if(!reverse_next_deletion) {
//                reverse_del_key->set_subject(dict->getMaxSubjectID());
//                reverse_del_key->set_predicate(dict->getMaxPredicateID());
//                reverse_del_key->set_object(dict->getMaxObjectID());
//            }
//            advance_reverse_del = false;
//        }
//
//        uint8_t comparison = compare();
//
//        if(IsBitSet(comparison, 3)){
//            advance_forward_add = true;
//        }
//        if(IsBitSet(comparison, 2)){
//            advance_forward_del = true;
//        }
//        if(IsBitSet(comparison, 1)){
//            advance_reverse_add = true;
//        }
//        if(IsBitSet(comparison, 0)){
//            advance_reverse_del = true;
//        }
//
//        if(comparison == 1 || comparison == 13){
//            // emit del
//            triple->setTriple(reverse_del_key);
//            triple->set_addition(false);
//            return true;
//        }
//        if(comparison == 4 || comparison == 7){
//            // emit del
//            triple->setTriple(forward_del_key);
//            triple->set_addition(false);
//            return true;
//        }
//        if(comparison == 14 || comparison == 2){
//            // emit add
//            triple->setTriple(reverse_add_key);
//            triple->set_addition(true);
//            return true;
//        }
//        if(comparison == 11 || comparison == 8){
//            // emit add
//            triple->setTriple(forward_add_key);
//            triple->set_addition(true);
//            return true;
//        }
//
//    }
//    return false;
}

//template<class DV>
//uint8_t BiDiffPatchTripleDeltaIterator<DV>::compare() {
//    PatchTreeKeyComparator *comp = forward_iterator->getComparator();
//    if (forward_next_deletion || forward_next_addition || reverse_next_deletion || forward_next_deletion) {
//        int comparison_forward = comp->compare(*forward_add_key, *forward_del_key);
//        int comparison_reverse = comp->compare(*reverse_add_key, *reverse_del_key);
//
//        if (comparison_forward == 0 && comparison_reverse == 0) {
//            int comparison2 = comp->compare(*forward_add_key, *reverse_add_key);
//
//            if (comparison2 == 0) {
//                // forward_add_key == forward_del_key == reverse_add_key == reverse_del_key
//                return 15; //1111
//            } else if (comparison2 < 0) {
//                // forward_add_key == forward_del_key < reverse_add_key == reverse_del_key
//                return 12; //1100
//            } else {
//                // forward_add_key == forward_del_key > reverse_add_key == reverse_del_key
//                return 3; //0011
//            }
//        } else if (comparison_forward == 0 && comparison_reverse < 0) {
//            int comparison2 = comp->compare(*forward_add_key, *reverse_add_key);
//            if (comparison2 == 0) {
//                // forward_add_key == forward_del_key == reverse_add_key < reverse_del_key
//                return 14; //1110
//            } else if (comparison2 < 0) {
//                // forward_add_key == forward_del_key < reverse_add_key < reverse_del_key
//                return 12; //1100
//            } else {
//                // forward_add_key == forward_del_key > reverse_add_key < reverse_del_key
//                return 2; //0010
//            }
//        } else if (comparison_forward == 0 && comparison_reverse > 0) {
//            int comparison2 = comp->compare(*forward_add_key, *reverse_del_key);
//            if (comparison2 == 0) {
//                // forward_add_key == forward_del_key == reverse_del_key < reverse_add_key
//                return 13; //1101
//            } else if (comparison2 < 0) {
//                // forward_add_key == forward_del_key < reverse_del_key < reverse_add_key
//                return 12; //1100
//            } else {
//                // forward_add_key == forward_del_key > reverse_del_key < reverse_add_key
//                return 1; //0001
//            }
//        } else if (comparison_forward < 0 && comparison_reverse == 0) {
//            int comparison2 = comp->compare(*forward_add_key, *reverse_add_key);
//            if (comparison2 == 0) {
//                // forward_del_key > forward_add_key == reverse_add_key == reverse_del_key
//                return 11; //1011
//            } else if (comparison2 < 0) {
//                // forward_del_key > forward_add_key < reverse_add_key == reverse_del_key
//                return 8; //1000
//            } else {
//                // forward_del_key > forward_add_key > reverse_add_key == reverse_del_key
//                return 3; //0011
//            }
//        } else if (comparison_forward < 0 && comparison_reverse < 0) {
//            int comparison2 = comp->compare(*forward_add_key, *reverse_add_key);
//            if (comparison2 == 0) {
//                // forward_del_key > forward_add_key == reverse_add_key < reverse_del_key
//                return 10; //1010
//            } else if (comparison2 < 0) {
//                // forward_del_key > forward_add_key < reverse_add_key < reverse_del_key
//                return 8; //1000
//            } else {
//                // forward_del_key > forward_add_key > reverse_add_key < reverse_del_key
//                return 4; //0010
//            }
//        } else if (comparison_forward < 0 && comparison_reverse > 0) {
//            int comparison2 = comp->compare(*forward_add_key, *reverse_del_key);
//            if (comparison2 == 0) {
//                // forward_del_key > forward_add_key == reverse_del_key < reverse_add_key
//                return 9; //1001
//            } else if (comparison2 < 0) {
//                // forward_del_key > forward_add_key < reverse_del_key < reverse_add_key
//                return 8; //1000
//            } else {
//                // forward_del_key > forward_add_key > reverse_del_key < reverse_add_key
//                return 1; //0001
//            }
//        } else if (comparison_forward > 0 && comparison_reverse == 0) {
//            int comparison2 = comp->compare(*forward_del_key, *reverse_del_key);
//            if (comparison2 == 0) {
//                // forward_add_key > forward_del_key == reverse_del_key == reverse_add_key
//                return 7; //0111
//            } else if (comparison2 < 0) {
//                // forward_add_key > forward_del_key < reverse_del_key == reverse_add_key                return 12; //1100
//                return 4; //0100
//            } else {
//                // forward_add_key > forward_del_key > reverse_del_key == reverse_add_key                return 1; //0001
//                return 3; //0011
//            }
//        } else if (comparison_forward > 0 && comparison_reverse < 0) {
//            int comparison2 = comp->compare(*forward_del_key, *reverse_add_key);
//            if (comparison2 == 0) {
//                // forward_add_key > forward_del_key == reverse_add_key < reverse_del_key
//                return 6; //0110
//            } else if (comparison2 < 0) {
//                // forward_add_key > forward_del_key < reverse_add_key < reverse_del_key                return 12; //1100
//                return 4; //0100
//            } else {
//                // forward_add_key > forward_del_key > reverse_add_key < reverse_del_key                return 1; //0001
//                return 2; //0010
//            }
//        } else if (comparison_forward > 0 && comparison_reverse > 0) {
//            int comparison2 = comp->compare(*forward_del_key, *reverse_del_key);
//            if (comparison2 == 0) {
//                // forward_add_key > forward_del_key == reverse_del_key < reverse_add_key
//                return 6; //0110
//            } else if (comparison2 < 0) {
//                // forward_add_key > forward_del_key < reverse_del_key < reverse_add_key                return 12; //1100
//                return 4; //0100
//            } else {
//                // forward_add_key > forward_del_key > reverse_del_key < reverse_add_key                return 1; //0001
//                return 1; //0001
//            }
//        }
//    } else {
//        return 0; //stop
//    }
//}
template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
template class ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
template class BiDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class BiDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
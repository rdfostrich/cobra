#include <dirent.h>
#include <iostream>
#include "patch_tree_manager.h"

PatchTreeManager::PatchTreeManager(string basePath, int8_t kc_opts, bool readonly) : basePath(basePath), loaded_patch_trees(detect_patch_trees()), kc_opts(kc_opts), readonly(readonly) {}

PatchTreeManager::~PatchTreeManager() {
    std::map<int, PatchTree*>::iterator it = loaded_patch_trees.begin();
    while(it != loaded_patch_trees.end()) {
        PatchTree* patchtree = it->second;
        if(patchtree != NULL) {
            delete patchtree;
        }
        it++;
    }
}

bool PatchTreeManager::append(PatchElementIterator *patch_it, int patch_id, int patch_tree_id, bool check_uniqueness,
                              ProgressListener *progressListener, DictionaryManager *dict) {
    PatchTree* patchtree = get_patch_tree(patch_tree_id, dict);
//    if(patch_tree_id < 0) {
//        patchtree = construct_next_patch_tree(patch_id, dict);
//    } else {
//        patchtree = get_patch_tree(patch_tree_id, dict);
//    }
    if (check_uniqueness) {
        return patchtree->append(patch_it, patch_id, progressListener);
    } else {
        patchtree->append_unsafe(patch_it, patch_id, progressListener);
        return true;
    }
}
bool PatchTreeManager::reverse_append(PatchElementIterator* patch_it, int patch_id, int patch_tree_id, DictionaryManager* dict, bool check_uniqueness, ProgressListener* progressListener) {
    PatchTree* patchtree;
    if(patch_tree_id < 0) {
        patchtree = construct_next_patch_tree(patch_id, dict);
    } else {
        patchtree = get_patch_tree(patch_tree_id, dict);
    }
    if (check_uniqueness) {
        return patchtree->reverse_append(patch_it, patch_id, progressListener);
    } else {
        patchtree->reverse_append_unsafe(patch_it, patch_id, progressListener);
        return true;
    }
}
bool PatchTreeManager::append(const PatchSorted &patch, int patch_id, int patch_tree_id, bool check_uniqueness,
                              ProgressListener *progressListener, DictionaryManager *dict) {
    PatchElementIteratorVector* it = new PatchElementIteratorVector(&patch.get_vector());
    bool ret = append(it, patch_id, patch_tree_id, check_uniqueness, progressListener, dict);
    delete it;
    return ret;
}
bool PatchTreeManager::reverse_append(const PatchSorted &patch, int patch_id, int patch_tree_id, DictionaryManager *dict, bool check_uniqueness,
                              ProgressListener *progressListener) {
    PatchElementIteratorVector* it = new PatchElementIteratorVector(&patch.get_vector());
    bool ret = reverse_append(it, patch_id, patch_tree_id, dict, check_uniqueness, progressListener);
    delete it;
    return ret;
}
std::map<int, PatchTree*> PatchTreeManager::detect_patch_trees() const {
    std::regex r("patchtree_([0-9]*).kct_spo_deletions");
    std::smatch base_match;
    std::map<int, PatchTree*> trees = std::map<int, PatchTree*>();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(basePath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string temp = std::string(ent->d_name);
            if(std::regex_match(temp, base_match, r)) {
                // The first sub_match is the whole string; the next
                // sub_match is the first parenthesized expression.
                if (base_match.size() == 2) {
                    std::ssub_match base_sub_match = base_match[1];
                    std::string base = (std::string) base_sub_match.str();
                    trees[std::stoi(base)] = NULL; // Don't load the actual file, we do this lazily
                }
            }
        }
        closedir(dir);
    }
    return trees;
}

const std::map<int, PatchTree*>& PatchTreeManager::get_patch_trees() const {
    return this->loaded_patch_trees;
}

PatchTree* PatchTreeManager::load_patch_tree(int patch_id_start, DictionaryManager* dict) {
    // TODO: We might want to look into unloading patch trees if they aren't used for a while. (using splay-tree/queue?)
    return loaded_patch_trees[patch_id_start] = new PatchTree(basePath, patch_id_start, dict, kc_opts, readonly);
}

PatchTree* PatchTreeManager::get_patch_tree(int patch_tree_id, DictionaryManager* dict) {
    if(patch_tree_id < 0) {
        return NULL;
    }
    std::map<int, PatchTree*>::iterator it = loaded_patch_trees.find(patch_tree_id);
    if(it == loaded_patch_trees.end()) { // tree not found
        return NULL;
    }
    PatchTree* patchtree = it->second;
    if(patchtree == NULL) {
        return load_patch_tree(it->first, dict);
    }
    return it->second;
}

PatchTree* PatchTreeManager::construct_next_patch_tree(int patch_id_start, DictionaryManager* dict) {
    return load_patch_tree(patch_id_start, dict);
}

Patch * PatchTreeManager::get_patch(int patch_id, int patch_tree_id, DictionaryManager *dict) {
    if(patch_tree_id < 0) {
        return new PatchSorted(dict);
    }
    return get_patch_tree(patch_tree_id, dict)->reconstruct_patch(patch_id, true);
}

int PatchTreeManager::get_max_patch_id(DictionaryManager* dict) {
    if (loaded_patch_trees.size() > 0) {
        std::map<int, PatchTree*>::const_iterator it = loaded_patch_trees.end();
        --it;
        PatchTree* patchTree = it->second;
        if (patchTree == NULL) {
            patchTree = load_patch_tree(it->first, dict);
        }
        return patchTree->get_max_patch_id();
    }
    return -1;
}

void PatchTreeManager::remove_patch_tree(int patch_tree_id){
    if(loaded_patch_trees[patch_tree_id] != NULL){
        delete loaded_patch_trees[patch_tree_id];
    }
    loaded_patch_trees.erase(patch_tree_id);
}
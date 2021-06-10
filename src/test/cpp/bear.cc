#include <iostream>
#include <kchashdb.h>
#include <HDT.hpp>
#include <thread>
#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/controller/controller.h"
#include "../../main/cpp/simpleprogresslistener.h"
#include "../../main/cpp/evaluate/Experimenter.h"
#include <chrono>
#define BASEURI "<http://example.org>"
#include <dirent.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>

using namespace std;
using namespace kyotocabinet;

vector<string> split(string data, string token) {
    vector<string> output;
    size_t pos = string::npos; // size_t to avoid improbable overflow
    do {
        pos = data.find(token);
        output.push_back(data.substr(0, pos));
        if (string::npos != pos)
            data = data.substr(pos + token.size());
    } while (string::npos != pos);
    return output;
}

string remove_brackets(string element) {
    if (element.at(0) == '<') {
        return element.substr(1, element.size() - 2);
    }
    if (element.at(0) == '?') {
        return "";
    }
    return element;
}

void all_VM_queries(Experimenter* experimenter, int start_id, int end_id){
    for(int i = start_id; i<=end_id; i++){
        cout << "-----version materialized-----" << endl;
        cout << "patch,lookup-ms-0-1,lookup-ms-0-50,lookup-ms-0-100,lookup-ms-100-1,lookup-ms-100-50,lookup-ms-100-100" << endl;
        long d0_1 = experimenter->test_lookup_version_materialized(experimenter->getController(), Triple("", "", "", experimenter->getController()->get_dictionary_manager(i)), 0, i, 1);
        long d0_50 = experimenter->test_lookup_version_materialized(experimenter->getController(), Triple("", "", "", experimenter->getController()->get_dictionary_manager(i)), 0, i, 50);
        long d0_100 = experimenter->test_lookup_version_materialized(experimenter->getController(), Triple("", "", "", experimenter->getController()->get_dictionary_manager(i)), 0, i, 100);

        long d100_1 = experimenter->test_lookup_version_materialized(experimenter->getController(), Triple("", "", "", experimenter->getController()->get_dictionary_manager(i)), 100, i, 1);
        long d100_50 = experimenter->test_lookup_version_materialized(experimenter->getController(), Triple("", "", "", experimenter->getController()->get_dictionary_manager(i)), 100, i, 50);
        long d100_100 = experimenter->test_lookup_version_materialized(experimenter->getController(), Triple("", "", "", experimenter->getController()->get_dictionary_manager(i)), 100, i, 100);
        cout << "" << i << ","
             << d0_1 << "," << d0_50 << "," << d0_100 << ","
             << d100_1 << "," << d100_50 << "," << d100_100
             << endl;

    }
}

void test_lookups_for_queries(int line_number, int patch_count, Experimenter& evaluator, string queriesFilePath, int replications) {

    std::ifstream queriesFile(queriesFilePath);
    std::string line;
    cout << "---QUERIES START: " << queriesFilePath << "---" << endl;
    while (std::getline(queriesFile, line)) {
        if(line_number > 0){
            line_number--;
        }
        else{
            vector<string> line_split = split(line, " ");
            evaluator.test_lookup(patch_count,
                                  remove_brackets(line_split[0]),
                                  remove_brackets(line_split[1]),
                                  remove_brackets(line_split[2]),
                    replications,
                    line_split.size() > 4 ? std::atoi(line_split[3].c_str()) : 0, // offset
                    line_split.size() > 4 ? std::atoi(line_split[4].c_str()) : -2 // limit
            );
        }
    }
    cout << "---QUERIES END---" << endl;
}

int main(int argc, char *argv[]) {
    std::cout << "running bear executable ..." << std::endl;
    std::cout << "expecting the following arguments: insertion option or query option, path, path to bear files or path to queries, number of patches, number of replications, line_nr " << std::endl;
    int number_of_patches = std::atoi(argv[4]);
    int number_of_replications = 1;
    int line_number = 0;

    std::string path_to_files = string(argv[3]);
    if(argc > 6){
        number_of_replications = std::atoi(argv[5]);
        line_number = std::atoi(argv[6]);
    }

    std::cout << "number of patches " << number_of_patches << endl;
        SimpleProgressListener *listener = new SimpleProgressListener();
    string basePath = string(argv[2]);
        Experimenter* experimenter = new Experimenter(string(argv[2]) + string(argv[1])+ ".txt", basePath, path_to_files, listener);
    if(string(argv[1]).compare("ostrich") == 0){
            std::cout << "inserting ostrich style" << std::endl;
            experimenter->populate_snapshot(0, 0, listener, Controller(basePath, TreeDB::TCOMPRESS));
            experimenter->populate_patch_tree(1, number_of_patches - 1, listener, false, -1,
                                              Controller(basePath, TreeDB::TCOMPRESS));
    }
    else if(string(argv[1]).compare("cobra_opt") == 0){
        std::cout << "inserting cobra_opt style" << std::endl;

        experimenter->populate_snapshot(number_of_patches / 2, number_of_patches / 2, listener, Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(number_of_patches / 2 - 1, 0, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(number_of_patches / 2 + 1, number_of_patches - 1, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
    }
    else if(string(argv[1]).compare("ostrich_opt") == 0){
        experimenter->populate_snapshot(0, 0, listener, Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(1, number_of_patches / 2 - 1, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_snapshot(number_of_patches / 2, number_of_patches / 2, listener, Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(number_of_patches / 2 + 1, number_of_patches - 1, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
    }
    else if (string(argv[1]).compare("query") == 0){
        test_lookups_for_queries(line_number, number_of_patches, *experimenter, path_to_files, number_of_replications);
    }
    else if (string(argv[1]).compare("pre_fix_up") == 0){
        experimenter->populate_snapshot(0, 0, listener, Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(1, number_of_patches / 2 - 1, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_snapshot(number_of_patches / 2 - 1, number_of_patches / 2 - 1, listener, Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(number_of_patches / 2, number_of_patches - 1, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
    }
    else if (string(argv[1]).compare("pre_fix_up_ostrich_opt") == 0){
        experimenter->populate_snapshot(0, 0, listener, Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(1, number_of_patches / 2, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_snapshot(number_of_patches / 2, number_of_patches / 2, listener, Controller(basePath, TreeDB::TCOMPRESS));
        experimenter->populate_patch_tree(number_of_patches / 2 + 1, number_of_patches - 1, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
    }
    else if (string(argv[1]).compare("fix_up") == 0){
        //extract changeset
        experimenter->getController()->extract_changeset(1, path_to_files);
        //delete tree
        experimenter->getController()->remove_forward_chain(basePath, 0);
        //reverse insert
        experimenter->populate_patch_tree(number_of_patches / 2 - 2, 0, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
    }
    else if (string(argv[1]).compare("fix_up_ostrich_opt") == 0){
        //extract changeset
        experimenter->extract_changeset(Controller(basePath, TreeDB::TCOMPRESS), 1, path_to_files);
        //delete tree
        experimenter->remove_forward_delta_chain(Controller(basePath, TreeDB::TCOMPRESS), basePath, 0);
        //reverse insert
        experimenter->populate_patch_tree(number_of_patches / 2 - 1, 0, listener, false, -1,
                                          Controller(basePath, TreeDB::TCOMPRESS));
    }
    else if (string(argv[1]).compare("print") == 0){
        cout << "debug only" << endl;
        experimenter->getController()->get_patch_tree_manager()->get_patch_tree(2, experimenter->getController()->get_dictionary_manager(2))->getTripleStore()->print_trees(experimenter->getController()->get_dictionary_manager(3));
        experimenter->getController()->get_patch_tree_manager()->get_patch_tree(4, experimenter->getController()->get_dictionary_manager(4))->getTripleStore()->print_trees(experimenter->getController()->get_dictionary_manager(5));

    }
    else if (string(argv[1]).compare("print_ostrich") == 0){
        experimenter->getController()->get_patch_tree_manager()->get_patch_tree(1, experimenter->getController()->get_dictionary_manager(3))->getTripleStore()->print_trees(experimenter->getController()->get_dictionary_manager(3));

    }
    delete listener;
    delete experimenter;
    return 0;

}

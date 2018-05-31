#include <iostream>
#include <kchashdb.h>
#include <HDT.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/controller/controller.h"
#include "../../main/cpp/evaluate/evaluator.h"
#include "../../main/cpp/simpleprogresslistener.h"

#define BASEURI "<http://example.org>"

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

void test_lookups_for_queries(int line_number, int patch_count, Evaluator& evaluator, string queriesFilePath, int replications) {
    string file_name = "";
    size_t i = queriesFilePath.rfind('/', queriesFilePath.length());
    if (i != string::npos) {
        file_name = (queriesFilePath.substr(i+1, queriesFilePath.length() - i));
    }
    std::ifstream queriesFile(queriesFilePath);
    std::string line;
    cout << "---QUERIES START: " << queriesFilePath << "---" << endl;
    while (std::getline(queriesFile, line)) {
        if(line_number > 0){
            line_number--;
        }
        else{
            vector<string> line_split = split(line, " ");
            evaluator.test_lookup(remove_brackets(line_split[0]), remove_brackets(line_split[1]),
                                  remove_brackets(line_split[2]), replications,
                                  line_split.size() > 4 ? std::atoi(line_split[3].c_str()) : 0,
                                  line_split.size() > 4 ? std::atoi(line_split[4].c_str()) : -2, file_name);
            break;
        }
    }
    cout << "---QUERIES END---" << endl;
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0] << " path_to_patches start_index end_index [path_to_queries_file replications line_number]" << endl;
        exit(1);
    }

    Evaluator evaluator;
    SimpleProgressListener* listener = new SimpleProgressListener();
    evaluator.init("./", argv[1], stoi(argv[2]), stoi(argv[3]), listener);
    delete listener;

    if (argc >= 6) {
        test_lookups_for_queries(stoi(argv[6]) ,stoi(argv[3]) - stoi(argv[2]) + 1 ,evaluator, ((std::string) argv[4]), stoi(argv[5]));
    }

    evaluator.cleanup_controller();
}

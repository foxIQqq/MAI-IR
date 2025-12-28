#include <iostream>
#include <fstream>
#include <string>
#include "../include/sch_containers.h"
#include "../include/sch_string_utils.h"

struct IndexData {
    SchVector<std::string> doc_names;
    SchStringHashMap<SchVector<int>> index;
};

IndexData load_index(const std::string& filename) {
    IndexData idx;
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "FATAL: Failed to open index file: " << filename << std::endl;
        exit(1);
    }

    size_t docs_count;
    if (!in.read(reinterpret_cast<char*>(&docs_count), sizeof(docs_count))) return idx;
    
    for (size_t i = 0; i < docs_count; ++i) {
        size_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        char* buffer = new char[len + 1];
        in.read(buffer, len);
        buffer[len] = '\0';
        idx.doc_names.push_back(std::string(buffer));
        delete[] buffer;
    }

    size_t vocab_size;
    in.read(reinterpret_cast<char*>(&vocab_size), sizeof(vocab_size));
    for (size_t i = 0; i < vocab_size; ++i) {
        size_t term_len;
        in.read(reinterpret_cast<char*>(&term_len), sizeof(term_len));
        char* term_buf = new char[term_len + 1];
        in.read(term_buf, term_len);
        term_buf[term_len] = '\0';
        std::string term(term_buf);
        delete[] term_buf;

        size_t list_size;
        in.read(reinterpret_cast<char*>(&list_size), sizeof(list_size));
        
        SchVector<int> postings;
        for (size_t j = 0; j < list_size; ++j) {
            int did;
            in.read(reinterpret_cast<char*>(&did), sizeof(did));
            postings.push_back(did);
        }
        idx.index.insert(term, postings);
    }
    return idx;
}

// БУЛЕВ ПОИСК: Пересечение (AND)
SchVector<int> intersect_lists(const SchVector<int>& l1, const SchVector<int>& l2) {
    SchVector<int> res;
    size_t i = 0, j = 0;
    while (i < l1.size() && j < l2.size()) {
        if (l1[i] == l2[j]) {
            res.push_back(l1[i]);
            i++; j++;
        } else if (l1[i] < l2[j]) {
            i++;
        } else {
            j++;
        }
    }
    return res;
}

// БУЛЕВ ПОИСК: Объединение (OR)
SchVector<int> union_lists(const SchVector<int>& l1, const SchVector<int>& l2) {
    SchVector<int> res;
    size_t i = 0, j = 0;
    while (i < l1.size() || j < l2.size()) {
        if (i == l1.size()) { res.push_back(l2[j++]); continue; }
        if (j == l2.size()) { res.push_back(l1[i++]); continue; }
        
        if (l1[i] == l2[j]) {
            res.push_back(l1[i]);
            i++; j++;
        } else if (l1[i] < l2[j]) {
            res.push_back(l1[i++]);
        } else {
            res.push_back(l2[j++]);
        }
    }
    return res;
}

SchVector<int> execute_query(const std::string& query, IndexData& idx) {
    std::stringstream ss(query);
    std::string word;
    SchVector<std::string> raw_parts;
    while(ss >> word) raw_parts.push_back(word);

    if (raw_parts.size() == 0) return SchVector<int>();

    auto process_term = [](const std::string& t) -> std::string {
        SchVector<std::string> tokens = tokenize(t);
        if (tokens.size() > 0) return stem_word(tokens[0]);
        return "";
    };

    std::string term = process_term(raw_parts[0]);
    SchVector<int>* ptr = idx.index.get(term);
    SchVector<int> result = (ptr) ? *ptr : SchVector<int>();

    for (size_t i = 1; i < raw_parts.size(); ++i) {
        std::string op = raw_parts[i];
        if (op == "AND" || op == "OR") {
            if (i + 1 >= raw_parts.size()) break;
            
            std::string next_term = process_term(raw_parts[i+1]);
            SchVector<int>* next_ptr = idx.index.get(next_term);
            SchVector<int> next_list = (next_ptr) ? *next_ptr : SchVector<int>();

            if (op == "AND") {
                result = intersect_lists(result, next_list);
            } else if (op == "OR") {
                result = union_lists(result, next_list);
            }
            i++;
        } else {
            std::string next_term = process_term(op);
            SchVector<int>* next_ptr = idx.index.get(next_term);
            SchVector<int> next_list = (next_ptr) ? *next_ptr : SchVector<int>();
            result = intersect_lists(result, next_list);
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    std::string index_path = "dumps/index.bin";
    if (argc > 1) {
        index_path = argv[1];
    }

    std::cerr << "Loading index from: " << index_path << "..." << std::endl;
    IndexData idx = load_index(index_path);
    std::cerr << "Index loaded. Ready for queries." << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        SchVector<int> results = execute_query(line, idx);
        
        std::cout << "Found " << results.size() << " documents:" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            if (i >= 15) { 
                std::cout << "... and " << (results.size() - 15) << " more" << std::endl; 
                break; 
            }
            std::cout << idx.doc_names[results[i]] << std::endl;
        }
        std::cout << "---END---" << std::endl;
    }
    return 0;
}
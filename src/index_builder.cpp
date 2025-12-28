#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include "../include/sch_containers.h"
#include "../include/sch_string_utils.h"

namespace fs = std::filesystem;

// Структуры
struct PostingList {
    SchVector<int> doc_ids;
    void add(int doc_id) {
        if (doc_ids.size() == 0 || doc_ids[doc_ids.size() - 1] != doc_id) {
            doc_ids.push_back(doc_id);
        }
    }
};

SchStringHashMap<PostingList> inverted_index(50000); // БУЛЕВ ИНДЕКС
SchVector<std::string> all_doc_names;
SchStringHashMap<int> term_frequencies; // ДАННЫЕ ДЛЯ ЗАКОНА ЦИПФА

void process_file(const std::string& filepath, int doc_id) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    SchVector<std::string> tokens = tokenize(content);
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::string stem = stem_word(tokens[i]);
        
        PostingList* plist = inverted_index.get(stem);
        if (plist == nullptr) {
            PostingList new_list;
            new_list.add(doc_id);
            inverted_index.insert(stem, new_list);
        } else {
            plist->add(doc_id);
        }

        int* freq = term_frequencies.get(stem);
        if (freq == nullptr) {
            term_frequencies.insert(stem, 1);
        } else {
            (*freq)++;
        }
    }
}

void save_index(const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open " << filename << " for writing." << std::endl;
        exit(1);
    }
    
    size_t docs_count = all_doc_names.size();
    out.write(reinterpret_cast<const char*>(&docs_count), sizeof(docs_count));
    for (size_t i = 0; i < docs_count; ++i) {
        size_t len = all_doc_names[i].size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(all_doc_names[i].c_str(), len);
    }

    SchVector<std::string> keys = inverted_index.get_keys();
    size_t vocab_size = keys.size();
    out.write(reinterpret_cast<const char*>(&vocab_size), sizeof(vocab_size));

    for (size_t i = 0; i < vocab_size; ++i) {
        std::string term = keys[i];
        PostingList* plist = inverted_index.get(term);
        
        size_t term_len = term.size();
        out.write(reinterpret_cast<const char*>(&term_len), sizeof(term_len));
        out.write(term.c_str(), term_len);
        
        size_t list_size = plist->doc_ids.size();
        out.write(reinterpret_cast<const char*>(&list_size), sizeof(list_size));
        
        for (size_t j = 0; j < list_size; ++j) {
            int did = plist->doc_ids[j];
            out.write(reinterpret_cast<const char*>(&did), sizeof(did));
        }
    }
    out.close();
}

void export_zipf(const std::string& filename) {
    std::ofstream out(filename);
    out << "Term,Frequency\n";
    SchVector<std::string> keys = term_frequencies.get_keys();
    for(size_t i=0; i<keys.size(); ++i) {
        out << keys[i] << "," << *term_frequencies.get(keys[i]) << "\n";
    }
    out.close();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <corpus_dir> <output_index_file>" << std::endl;
        return 1;
    }

    std::string corpus_dir = argv[1];
    std::string index_file = argv[2];
    
    if (!fs::exists(corpus_dir)) {
        std::cerr << "Error: Corpus directory not found." << std::endl;
        return 1;
    }

    int doc_id_counter = 0;
    std::cout << "Indexing documents from: " << corpus_dir << std::endl;

    for (const auto& entry : fs::directory_iterator(corpus_dir)) {
        if (entry.path().extension() == ".txt") {
            all_doc_names.push_back(entry.path().filename().string());
            process_file(entry.path().string(), doc_id_counter);
            doc_id_counter++;
            
            if (doc_id_counter % 2000 == 0) 
                std::cout << "Processed " << doc_id_counter << " files..." << std::endl;
        }
    }

    std::cout << "Total processed: " << doc_id_counter << std::endl;
    std::cout << "Saving index to: " << index_file << std::endl;
    save_index(index_file);
    
    std::string zipf_file = index_file + ".csv";
    std::cout << "Exporting Zipf stats to: " << zipf_file << std::endl;
    export_zipf(zipf_file);

    std::cout << "Done." << std::endl;
    return 0;
}
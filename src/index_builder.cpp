#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include "../include/sch_containers.h"
#include "../include/sch_string_utils.h"
#include "../include/sch_string.h"

struct PostingList {
    SchVector<int> doc_ids;
    void add(int doc_id) {
        if (doc_ids.size() == 0 || doc_ids[doc_ids.size() - 1] != doc_id) doc_ids.push_back(doc_id);
    }
};

SchStringHashMap<PostingList> inverted_index(50000);
SchVector<SchString> all_doc_names;
SchStringHashMap<int> term_frequencies(50007);

template <typename T, typename Comp>
void quicksort(SchVector<T>& arr, int left, int right, Comp comp) {
    if (left >= right) return;
    int i = left, j = right;
    T pivot = arr[(left + right) / 2];
    while (i <= j) {
        while (comp(arr[i], pivot)) ++i;
        while (comp(pivot, arr[j])) --j;
        if (i <= j) {
            T tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
            ++i; --j;
        }
    }
    if (left < j) quicksort(arr, left, j, comp);
    if (i < right) quicksort(arr, i, right, comp);
}

void sort_int_vector(SchVector<int>& v) {
    if (v.size() <= 1) return;
    auto comp = [](const int& a, const int& b){ return a < b; };
    quicksort<int, decltype(comp)>(v, 0, (int)v.size() - 1, comp);
}

void sort_schstring_vector(SchVector<SchString>& v) {
    if (v.size() <= 1) return;
    auto comp = [](const SchString& a, const SchString& b){ return std::strcmp(a.c_str(), b.c_str()) < 0; };
    quicksort<SchString, decltype(comp)>(v, 0, (int)v.size() - 1, comp);
}

static std::string read_file_to_string(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return std::string();
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return std::string(); }
    std::string out;
    out.resize(sz);
    size_t r = fread(&out[0], 1, sz, f);
    fclose(f);
    if (r != (size_t)sz) return std::string();
    return out;
}

void process_file(const char* filepath, int doc_id) {
    std::string content = read_file_to_string(filepath);
    if (content.empty()) return;
    SchString content_sch(content.c_str(), content.size());
    SchVector<SchString> tokens = tokenize(content_sch, 1);
    for (size_t i = 0; i < tokens.size(); ++i) {
        SchString stem = stem_word(tokens[i]);
        PostingList* plist = inverted_index.get(stem);
        if (plist == nullptr) {
            PostingList new_list;
            new_list.add(doc_id);
            inverted_index.insert(stem, new_list);
        } else {
            plist->add(doc_id);
        }
        int* freq = term_frequencies.get(stem);
        if (freq == nullptr) term_frequencies.insert(stem, 1);
        else (*freq)++;
    }
}

void save_index(const char* filename) {
    SchVector<SchString> keys = inverted_index.get_keys();
    sort_schstring_vector(keys);

    FILE* out = fopen(filename, "wb");
    if (!out) { fprintf(stderr, "Error: cannot open %s for writing\n", filename); exit(1); }

    size_t docs_count = all_doc_names.size();
    fwrite(&docs_count, sizeof(docs_count), 1, out);
    for (size_t i = 0; i < docs_count; ++i) {
        SchString s = all_doc_names[i];
        size_t len = s.size();
        fwrite(&len, sizeof(len), 1, out);
        fwrite(s.c_str(), 1, len, out);
    }

    size_t vocab_size = keys.size();
    fwrite(&vocab_size, sizeof(vocab_size), 1, out);
    for (size_t i = 0; i < vocab_size; ++i) {
        SchString term = keys[i];
        PostingList* plist = inverted_index.get(term);
        sort_int_vector(plist->doc_ids);

        size_t term_len = term.size();
        fwrite(&term_len, sizeof(term_len), 1, out);
        fwrite(term.c_str(), 1, term_len, out);

        size_t list_size = plist->doc_ids.size();
        fwrite(&list_size, sizeof(list_size), 1, out);
        for (size_t j = 0; j < list_size; ++j) {
            int did = plist->doc_ids[j];
            fwrite(&did, sizeof(did), 1, out);
        }
    }
    fclose(out);
}

void export_zipf(const char* filename) {
    SchVector<SchString> keys = term_frequencies.get_keys();
    SchVector< SchPair<SchString,int> > arr;
    for (size_t i = 0; i < keys.size(); ++i) {
        int* f = term_frequencies.get(keys[i]);
        if (f) arr.push_back(SchPair<SchString,int>(keys[i], *f));
    }
    auto comp = [](const SchPair<SchString,int>& a, const SchPair<SchString,int>& b){ return a.value > b.value; };
    if (arr.size() > 1) quicksort< SchPair<SchString,int>, decltype(comp) >(arr, 0, (int)arr.size() - 1, comp);

    FILE* out = fopen(filename, "w");
    if (!out) { fprintf(stderr, "Error: cannot open %s for writing\n", filename); return; }
    fprintf(out, "Term,Frequency\n");
    for (size_t i = 0; i < arr.size(); ++i) {
        fprintf(out, "%s,%d\n", arr[i].key.c_str(), arr[i].value);
    }
    fclose(out);
}

SchVector<SchString> list_txt_files(const char* dirpath) {
    SchVector<SchString> files;
    DIR* dir = opendir(dirpath);
    if (!dir) return files;
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG || ent->d_type == DT_UNKNOWN) {
            const char* name = ent->d_name;
            size_t ln = std::strlen(name);
            if (ln > 4) {
                if (name[ln-4]=='.' && name[ln-3]=='t' && name[ln-2]=='x' && name[ln-1]=='t') {
                    size_t base_len = std::strlen(dirpath);
                    char* full = new char[base_len + 1 + ln + 1];
                    std::strcpy(full, dirpath);
                    if (dirpath[base_len-1] != '/') strcat(full, "/");
                    strcat(full, name);
                    files.push_back(SchString(full));
                    delete[] full;
                }
            }
        }
    }
    closedir(dir);
    return files;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <corpus_dir> <output_index_file>\n", argv[0]);
        return 1;
    }
    const char* corpus_dir = argv[1];
    const char* index_file = argv[2];

    SchVector<SchString> files = list_txt_files(corpus_dir);
    sort_schstring_vector(files);

    int doc_id_counter = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        const char* path = files[i].c_str();
        const char* p = files[i].c_str();
        const char* last_slash = std::strrchr(p, '/');
        SchString filename = last_slash ? SchString(last_slash + 1) : SchString(p);
        all_doc_names.push_back(filename);
        process_file(path, doc_id_counter);
        doc_id_counter++;
        if (doc_id_counter % 2000 == 0) fprintf(stderr, "Processed %d files...\n", doc_id_counter);
    }

    fprintf(stderr, "Total processed: %d\n", doc_id_counter);
    fprintf(stderr, "Saving index to: %s\n", index_file);
    save_index(index_file);

    std::string zipf = std::string(index_file) + ".csv";
    export_zipf(zipf.c_str());

    fprintf(stderr, "Done.\n");
    return 0;
}

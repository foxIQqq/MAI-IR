#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iostream>
#include "../include/sch_containers.h"
#include "../include/sch_string.h"
#include "../include/sch_string_utils.h"

struct IndexData {
    SchVector<SchString> doc_names;
    SchStringHashMap< SchVector<int> > index;
};

IndexData load_index(const char* filename) {
    IndexData idx;
    FILE* in = fopen(filename, "rb");
    if (!in) { fprintf(stderr, "FATAL: Failed to open index file: %s\n", filename); exit(1); }

    size_t docs_count = 0;
    if (fread(&docs_count, sizeof(docs_count), 1, in) != 1) { fclose(in); return idx; }

    for (size_t i = 0; i < docs_count; ++i) {
        size_t len;
        fread(&len, sizeof(len), 1, in);
        char* buffer = new char[len + 1];
        fread(buffer, 1, len, in);
        buffer[len] = '\0';
        idx.doc_names.push_back(SchString(buffer, len));
        delete[] buffer;
    }

    size_t vocab_size = 0;
    fread(&vocab_size, sizeof(vocab_size), 1, in);
    for (size_t i = 0; i < vocab_size; ++i) {
        size_t term_len;
        fread(&term_len, sizeof(term_len), 1, in);
        char* term_buf = new char[term_len + 1];
        fread(term_buf, 1, term_len, in);
        term_buf[term_len] = '\0';
        SchString term(term_buf, term_len);
        delete[] term_buf;

        size_t list_size;
        fread(&list_size, sizeof(list_size), 1, in);
        SchVector<int> postings;
        for (size_t j = 0; j < list_size; ++j) {
            int did;
            fread(&did, sizeof(did), 1, in);
            postings.push_back(did);
        }
        idx.index.insert(term, postings);
    }
    fclose(in);
    return idx;
}

SchVector<int> intersect_lists(const SchVector<int>& l1, const SchVector<int>& l2) {
    SchVector<int> res;
    size_t i = 0, j = 0;
    while (i < l1.size() && j < l2.size()) {
        if (l1[i] == l2[j]) { res.push_back(l1[i]); i++; j++; }
        else if (l1[i] < l2[j]) i++;
        else j++;
    }
    return res;
}
SchVector<int> union_lists(const SchVector<int>& l1, const SchVector<int>& l2) {
    SchVector<int> res;
    size_t i = 0, j = 0;
    while (i < l1.size() || j < l2.size()) {
        if (i == l1.size()) { res.push_back(l2[j++]); continue; }
        if (j == l2.size()) { res.push_back(l1[i++]); continue; }
        if (l1[i] == l2[j]) { res.push_back(l1[i]); i++; j++; }
        else if (l1[i] < l2[j]) res.push_back(l1[i++]);
        else res.push_back(l2[j++]);
    }
    return res;
}

void to_upper_inplace(char* s) {
    for (size_t i = 0; s[i]; ++i) s[i] = (char)toupper((unsigned char)s[i]);
}

SchVector<int> execute_query_cstr(const char* query_cstr, IndexData& idx) {
    SchVector<const char*> parts;
    char* qcopy = strdup(query_cstr);
    char* tok = std::strtok(qcopy, " \t\r\n");
    while (tok) { parts.push_back(tok); tok = std::strtok(NULL, " \t\r\n"); }
    if (parts.size() == 0) { free(qcopy); return SchVector<int>(); }

    auto process_term = [&](const char* t)->SchVector<int> {
        SchString t_sch(t);
        SchVector<SchString> toks = tokenize(t_sch);
        if (toks.size() == 0) return SchVector<int>();
        SchString st = stem_word(toks[0]);
        SchVector<int>* ptr = idx.index.get(st);
        if (ptr) return *ptr;
        return SchVector<int>();
    };

    SchVector<int> result = process_term(parts[0]);
    for (size_t i = 1; i < parts.size(); ++i) {
        const char* op = parts[i];
        char op_copy[16]; std::strncpy(op_copy, op, 15); op_copy[15] = '\0';
        to_upper_inplace(op_copy);
        if (std::strcmp(op_copy, "AND") == 0 || std::strcmp(op_copy, "OR") == 0) {
            if (i + 1 >= parts.size()) break;
            SchVector<int> next = process_term(parts[i+1]);
            if (std::strcmp(op_copy, "AND") == 0) result = intersect_lists(result, next);
            else result = union_lists(result, next);
            ++i;
        } else {
            SchVector<int> next = process_term(op);
            result = intersect_lists(result, next);
        }
    }
    free(qcopy);
    return result;
}

int main(int argc, char* argv[]) {
    const char* index_path = "dumps/main_index.bin";
    if (argc > 1) index_path = argv[1];

    fprintf(stderr, "Loading index from: %s ...\n", index_path);
    IndexData idx = load_index(index_path);
    fprintf(stderr, "Index loaded. Ready for queries.\n");

    char linebuf[4096];
    while (fgets(linebuf, sizeof(linebuf), stdin)) {
        size_t L = strlen(linebuf);
        while (L > 0 && (linebuf[L-1] == '\n' || linebuf[L-1] == '\r')) { linebuf[L-1] = '\0'; --L; }
        if (L == 0) continue;
        SchVector<int> results = execute_query_cstr(linebuf, idx);
        printf("Found %zu documents:\n", results.size());
        for (size_t i = 0; i < results.size(); ++i) {
            if (i >= 15) { printf("... and %zu more\n", results.size() - 15); break; }
            int docid = results[i];
            if (docid >= 0 && docid < (int)idx.doc_names.size()) {
                printf("%s\n", idx.doc_names[docid].c_str());
            } else {
                printf("(doc id %d)\n", docid);
            }
        }
        printf("---END---\n");
        fflush(stdout);
    }
    return 0;
}

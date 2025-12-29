#ifndef SCH_STRING_UTILS_HPP
#define SCH_STRING_UTILS_HPP

#include "sch_containers.h"
#include "sch_string.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>

inline bool ends_with_cstr(const char* s, const char* suf) {
    size_t ls = std::strlen(s);
    size_t rs = std::strlen(suf);
    if (ls < rs) return false;
    return std::strcmp(s + (ls - rs), suf) == 0;
}

SchVector<SchString> tokenize(const SchString& text_sch, size_t min_token_len = 1) {
    std::string text(text_sch.c_str(), text_sch.size());
    SchVector<SchString> tokens;
    std::string clean;
    clean.reserve(text.size());
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) clean.push_back(std::tolower(static_cast<unsigned char>(c)));
        else clean.push_back(' ');
    }
    std::stringstream ss(clean);
    std::string w;
    while (ss >> w) {
        if (w.size() >= min_token_len) tokens.push_back(SchString(w.c_str(), w.size()));
    }
    return tokens;
}

static SchString stem_word(const SchString& word_sch) {
    std::string word(word_sch.c_str(), word_sch.size());
    if (word.size() <= 2) return SchString(word.c_str(), word.size());
    std::string s;
    s.reserve(word.size());
    for (char c : word) s.push_back(std::tolower(static_cast<unsigned char>(c)));

    std::function<bool(const std::string&, int)> is_consonant = [&](const std::string& str, int i)->bool {
        char ch = str[i];
        if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u') return false;
        if (ch == 'y') {
            if (i == 0) return true;
            return !is_consonant(str, i - 1);
        }
        return true;
    };

    std::function<int(const std::string&)> measure = [&](const std::string& str)->int {
        int m = 0;
        size_t i = 0, n = str.size();
        while (i < n && is_consonant(str, static_cast<int>(i))) ++i;
        while (i < n) {
            while (i < n && !is_consonant(str, static_cast<int>(i))) ++i;
            while (i < n && is_consonant(str, static_cast<int>(i))) ++i;
            if (i > 0) m++;
        }
        return m;
    };

    std::function<bool(const std::string&)> contains_vowel = [&](const std::string& str)->bool {
        for (size_t i = 0; i < str.size(); ++i) if (!is_consonant(str, static_cast<int>(i))) return true;
        return false;
    };
    std::function<bool(const std::string&)> ends_with_double_consonant = [&](const std::string& str)->bool {
        size_t n = str.size();
        if (n >= 2 && str[n-1] == str[n-2]) return is_consonant(str, static_cast<int>(n-1));
        return false;
    };
    std::function<bool(const std::string&, int)> cvc = [&](const std::string& str, int i)->bool {
        if (i < 2) return false;
        if (!is_consonant(str, i) || is_consonant(str, i-1) || !is_consonant(str, i-2)) return false;
        char ch = str[i];
        if (ch == 'w' || ch == 'x' || ch == 'y') return false;
        return true;
    };

    if (ends_with_cstr(s.c_str(), "sses")) s = s.substr(0, s.size() - 2);
    else if (ends_with_cstr(s.c_str(), "ies")) s = s.substr(0, s.size() - 2);
    else if (ends_with_cstr(s.c_str(), "ss")) {}
    else if (ends_with_cstr(s.c_str(), "s")) s = s.substr(0, s.size() - 1);

    bool step1b_performed = false;
    if (ends_with_cstr(s.c_str(), "eed")) {
        std::string stem = s.substr(0, s.size() - 3);
        if (measure(stem) > 0) s = stem + "ee";
    } else if (ends_with_cstr(s.c_str(), "ed")) {
        std::string stem = s.substr(0, s.size() - 2);
        if (contains_vowel(stem)) { s = stem; step1b_performed = true; }
    } else if (ends_with_cstr(s.c_str(), "ing")) {
        std::string stem = s.substr(0, s.size() - 3);
        if (contains_vowel(stem)) { s = stem; step1b_performed = true; }
    }

    if (step1b_performed) {
        if (ends_with_cstr(s.c_str(), "at") || ends_with_cstr(s.c_str(), "bl") || ends_with_cstr(s.c_str(), "iz")) { s.push_back('e'); }
        else if (ends_with_double_consonant(s)) {
            char last = s.back();
            if (last != 'l' && last != 's' && last != 'z') s = s.substr(0, s.size() - 1);
        } else if (measure(s) == 1 && cvc(s, static_cast<int>(s.size()) - 1)) s.push_back('e');
    }

    if (ends_with_cstr(s.c_str(), "y")) {
        std::string stem = s.substr(0, s.size() - 1);
        if (contains_vowel(stem)) s = stem + "i";
    }

    struct Suf { const char* suf; const char* rep; };
    static Suf step2[] = {
        {"ational","ate"},{"tional","tion"},{"enci","ence"},{"anci","ance"},
        {"izer","ize"},{"abli","able"},{"alli","al"},{"entli","ent"},
        {"eli","e"},{"ousli","ous"},{"ization","ize"},{"ation","ate"},
        {"ator","ate"},{"alism","al"},{"iveness","ive"},{"fulness","ful"},
        {"ousness","ous"},{"aliti","al"},{"iviti","ive"},{"biliti","ble"},
        {nullptr,nullptr}
    };
    for (int i = 0; step2[i].suf; ++i) {
        if (ends_with_cstr(s.c_str(), step2[i].suf)) {
            std::string stem = s.substr(0, s.size() - std::strlen(step2[i].suf));
            if (measure(stem) > 0) { s = stem + std::string(step2[i].rep); }
            break;
        }
    }
    static Suf step3[] = { {"icate","ic"},{"ative",""},{"alize","al"},{"iciti","ic"},{"ical","ic"},{"ful",""},{"ness",""},{nullptr,nullptr} };
    for (int i = 0; step3[i].suf; ++i) {
        if (ends_with_cstr(s.c_str(), step3[i].suf)) {
            std::string stem = s.substr(0, s.size() - std::strlen(step3[i].suf));
            if (measure(stem) > 0) s = stem + std::string(step3[i].rep);
            break;
        }
    }
    const char* step4_list[] = {"al","ance","ence","er","ic","able","ible","ant","ement","ment","ent","ion","ou","ism","ate","iti","ous","ive","ize", nullptr};
    for (int i = 0; step4_list[i]; ++i) {
        if (ends_with_cstr(s.c_str(), step4_list[i])) {
            std::string stem = s.substr(0, s.size() - std::strlen(step4_list[i]));
            if (measure(stem) > 1) s = stem;
            break;
        }
    }
    if (ends_with_cstr(s.c_str(), "e")) {
        std::string stem = s.substr(0, s.size() - 1);
        if (measure(stem) > 1 || (measure(stem) == 1 && !cvc(stem, static_cast<int>(stem.size()) - 1))) s = stem;
    }

    return SchString(s.c_str(), s.size());
}

#endif

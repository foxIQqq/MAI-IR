#ifndef SCH_STRING_UTILS_HPP
#define SCH_STRING_UTILS_HPP

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "sch_containers.h"

bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool has_vowel(const std::string& str) {
    for (char c : str) {
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y') return true;
    }
    return false;
}

// СТЕММИНГ (Stemming)
std::string stem_word(const std::string& word) {
    std::string s = word;
    if (s.length() <= 2) return s;

    // Step 1a
    if (ends_with(s, "sses")) s = s.substr(0, s.length() - 2);
    else if (ends_with(s, "ies")) s = s.substr(0, s.length() - 2); // ties -> ti
    else if (ends_with(s, "ss")) {} // do nothing
    else if (ends_with(s, "s")) s = s.substr(0, s.length() - 1);

    // Step 1b
    bool extra_step = false;
    if (ends_with(s, "eed")) {
        std::string stem = s.substr(0, s.length() - 3);
        if (stem.length() > 0) s = stem + "ee"; 
    } 
    else if (ends_with(s, "ed")) {
        std::string stem = s.substr(0, s.length() - 2);
        if (has_vowel(stem)) { s = stem; extra_step = true; }
    } 
    else if (ends_with(s, "ing")) {
        std::string stem = s.substr(0, s.length() - 3);
        if (has_vowel(stem)) { s = stem; extra_step = true; }
    }

    // Очистка после 1b (например, hopping -> hopp -> hop)
    if (extra_step) {
        if (ends_with(s, "at") || ends_with(s, "bl") || ends_with(s, "iz")) {
            s += "e";
        } else if (s.length() >= 2 && s[s.length()-1] == s[s.length()-2] && 
                   s.back() != 'l' && s.back() != 's' && s.back() != 'z') {
            s = s.substr(0, s.length() - 1);
        }
    }
    
    // Step 1c (happy -> happi)
    if (ends_with(s, "y")) {
        std::string stem = s.substr(0, s.length() - 1);
        if (has_vowel(stem)) s = stem + "i";
    }

    return s;
}

// ТОКЕНИЗАЦИЯ (Tokenization)
// Превращает текст в список слов, убирая мусор и приводя к нижнему регистру.
SchVector<std::string> tokenize(const std::string& text) {
    SchVector<std::string> tokens;
    std::string clean_text;
    
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            clean_text += std::tolower(static_cast<unsigned char>(c));
        } else {
            clean_text += ' ';
        }
    }
    
    std::stringstream ss(clean_text);
    std::string segment;
    while (ss >> segment) {
        if (!segment.empty()) {
            tokens.push_back(segment);
        }
    }
    return tokens;
}

#endif
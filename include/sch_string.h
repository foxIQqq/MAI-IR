#ifndef SCH_STRING_H
#define SCH_STRING_H

#include <cstring>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>

class SchString {
private:
    char* data_;
    size_t len_;
public:
    SchString() : data_(nullptr), len_(0) {}
    SchString(const char* s) {
        if (!s) { data_ = nullptr; len_ = 0; return; }
        len_ = std::strlen(s);
        data_ = new char[len_ + 1];
        std::memcpy(data_, s, len_ + 1);
    }
    SchString(const char* s, size_t n) {
        if (!s || n == 0) { data_ = nullptr; len_ = 0; return; }
        len_ = n;
        data_ = new char[len_ + 1];
        std::memcpy(data_, s, len_);
        data_[len_] = '\0';
    }
    SchString(const SchString& other) {
        if (other.len_ == 0) { data_ = nullptr; len_ = 0; return; }
        len_ = other.len_;
        data_ = new char[len_ + 1];
        std::memcpy(data_, other.data_, len_ + 1);
    }
    SchString& operator=(const SchString& other) {
        if (this == &other) return *this;
        delete[] data_;
        if (other.len_ == 0) { data_ = nullptr; len_ = 0; return *this; }
        len_ = other.len_;
        data_ = new char[len_ + 1];
        std::memcpy(data_, other.data_, len_ + 1);
        return *this;
    }
    ~SchString() {
        delete[] data_;
    }

    static SchString from_std_string(const std::string& s) {
        return SchString(s.c_str(), s.size());
    }

    const char* c_str() const { return (data_ ? data_ : ""); }
    size_t size() const { return len_; }
    bool empty() const { return len_ == 0; }

    bool operator==(const SchString& other) const {
        if (len_ != other.len_) return false;
        if (len_ == 0) return true;
        return std::memcmp(data_, other.data_, len_) == 0;
    }
    bool operator!=(const SchString& other) const { return !(*this == other); }

    int compare(const SchString& other) const {
        if (len_ == 0 && other.len_ == 0) return 0;
        if (len_ == 0) return -1;
        if (other.len_ == 0) return 1;
        return std::strcmp(this->c_str(), other.c_str());
    }

    char* strdup_c() const {
        if (!data_) {
            char* out = new char[1];
            out[0] = '\0';
            return out;
        }
        char* out = new char[len_ + 1];
        std::memcpy(out, data_, len_ + 1);
        return out;
    }
};

#endif

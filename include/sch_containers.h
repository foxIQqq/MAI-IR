#ifndef SCH_CONTAINERS_HPP
#define SCH_CONTAINERS_HPP

#include <cstddef>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include "sch_string.h"

template <typename T>
class SchVector {
private:
    T* data;
    size_t capacity;
    size_t length;
    void resize(size_t new_capacity) {
        T* new_data = new T[new_capacity];
        for (size_t i = 0; i < length; ++i) {
            new_data[i] = data[i];
        }
        delete[] data;
        data = new_data;
        capacity = new_capacity;
    }
public:
    SchVector() : capacity(10), length(0) { data = new T[capacity]; }
    ~SchVector() { delete[] data; }
    SchVector(const SchVector& other) : capacity(other.capacity), length(other.length) {
        data = new T[capacity];
        for (size_t i = 0; i < length; ++i) data[i] = other.data[i];
    }
    SchVector& operator=(const SchVector& other) {
        if (this != &other) {
            delete[] data;
            capacity = other.capacity;
            length = other.length;
            data = new T[capacity];
            for (size_t i = 0; i < length; ++i) data[i] = other.data[i];
        }
        return *this;
    }

    void push_back(const T& value) {
        if (length == capacity) resize(capacity * 2);
        data[length++] = value;
    }
    size_t size() const { return length; }
    T& operator[](size_t index) {
        if (index >= length) throw std::out_of_range("Index out of bounds");
        return data[index];
    }
    const T& operator[](size_t index) const {
        if (index >= length) throw std::out_of_range("Index out of bounds");
        return data[index];
    }
    T* begin() { return data; }
    T* end() { return data + length; }
    void clear() { length = 0; }
};

template <typename K, typename V>
struct SchPair {
    K key;
    V value;
    SchPair() {}
    SchPair(K k, V v) : key(k), value(v) {}
};

template <typename V>
class SchStringHashMap {
private:
    struct Node {
        char* key; // C-string copied
        V value;
        Node* next;
        Node(const char* k, const V& v) : value(v), next(nullptr) {
            size_t ln = std::strlen(k);
            key = new char[ln + 1];
            std::memcpy(key, k, ln + 1);
        }
        ~Node() { delete[] key; }
    };

    Node** buckets;
    size_t bucket_count;
    size_t size_;

    size_t hash_cstr(const char* key) const {
        unsigned long h = 2166136261u;
        for (const unsigned char* p = (const unsigned char*)key; *p; ++p) {
            h = (h ^ (*p)) * 16777619u;
        }
        return (size_t)(h % bucket_count);
    }

public:
    SchStringHashMap(size_t buckets_init = 10007) : bucket_count(buckets_init), size_(0) {
        buckets = new Node*[bucket_count];
        for (size_t i = 0; i < bucket_count; ++i) buckets[i] = nullptr;
    }
    ~SchStringHashMap() {
        for (size_t i = 0; i < bucket_count; ++i) {
            Node* cur = buckets[i];
            while (cur) {
                Node* tmp = cur;
                cur = cur->next;
                delete tmp;
            }
        }
        delete[] buckets;
    }

    void insert(const SchString& key, const V& value) {
        const char* k = key.c_str();
        size_t h = hash_cstr(k);
        Node* cur = buckets[h];
        while (cur) {
            if (std::strcmp(cur->key, k) == 0) {
                cur->value = value;
                return;
            }
            cur = cur->next;
        }
        Node* node = new Node(k, value);
        node->next = buckets[h];
        buckets[h] = node;
        size_++;
    }

    V* get(const SchString& key) {
        const char* k = key.c_str();
        size_t h = hash_cstr(k);
        Node* cur = buckets[h];
        while (cur) {
            if (std::strcmp(cur->key, k) == 0) return &(cur->value);
            cur = cur->next;
        }
        return nullptr;
    }

    SchVector<SchString> get_keys() const {
        SchVector<SchString> keys;
        for (size_t i = 0; i < bucket_count; ++i) {
            Node* cur = buckets[i];
            while (cur) {
                keys.push_back(SchString(cur->key));
                cur = cur->next;
            }
        }
        return keys;
    }

    size_t size() const { return size_; }
};

#endif

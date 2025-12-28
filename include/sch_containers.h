#ifndef SCH_CONTAINERS_HPP
#define SCH_CONTAINERS_HPP

#include <cstddef>
#include <stdexcept>
#include <iostream>

// --- Аналог std::vector ---
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
    SchVector() : capacity(10), length(0) {
        data = new T[capacity];
    }

    ~SchVector() {
        delete[] data;
    }

    SchVector(const SchVector& other) : capacity(other.capacity), length(other.length) {
        data = new T[capacity];
        for (size_t i = 0; i < length; ++i) {
            data[i] = other.data[i];
        }
    }
    
    SchVector& operator=(const SchVector& other) {
        if (this != &other) {
            delete[] data;
            capacity = other.capacity;
            length = other.length;
            data = new T[capacity];
            for (size_t i = 0; i < length; ++i) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    void push_back(const T& value) {
        if (length == capacity) {
            resize(capacity * 2);
        }
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
    
    void clear() {
        length = 0;
    }
};

// --- Аналог std::pair ---
template <typename K, typename V>
struct SchPair {
    K key;
    V value;
    SchPair() {}
    SchPair(K k, V v) : key(k), value(v) {}
};

// --- Простой хэш-мап (метод цепочек) ---
// Ограничение: K должен быть std::string (так как нужен хэшер) или типом, который можно привести к int
template <typename V>
class SchStringHashMap {
private:
    struct Node {
        SchPair<std::string, V> data;
        Node* next;
        Node(std::string k, V v) : data(k, v), next(nullptr) {}
    };

    Node** buckets;
    size_t bucket_count;
    size_t size_;

    size_t hash(const std::string& key) const {
        size_t h = 0;
        for (char c : key) {
            h = 31 * h + c;
        }
        return h % bucket_count;
    }

public:
    SchStringHashMap(size_t buckets_init = 10007) : bucket_count(buckets_init), size_(0) {
        buckets = new Node*[bucket_count];
        for (size_t i = 0; i < bucket_count; ++i) buckets[i] = nullptr;
    }

    ~SchStringHashMap() {
        for (size_t i = 0; i < bucket_count; ++i) {
            Node* current = buckets[i];
            while (current) {
                Node* temp = current;
                current = current->next;
                delete temp;
            }
        }
        delete[] buckets;
    }

    void insert(const std::string& key, const V& value) {
        size_t h = hash(key);
        Node* current = buckets[h];
        while (current) {
            if (current->data.key == key) {
                current->data.value = value;
                return;
            }
            current = current->next;
        }
        Node* newNode = new Node(key, value);
        newNode->next = buckets[h];
        buckets[h] = newNode;
        size_++;
    }

    V* get(const std::string& key) {
        size_t h = hash(key);
        Node* current = buckets[h];
        while (current) {
            if (current->data.key == key) {
                return &(current->data.value);
            }
            current = current->next;
        }
        return nullptr;
    }

    SchVector<std::string> get_keys() const {
        SchVector<std::string> keys;
        for (size_t i = 0; i < bucket_count; ++i) {
            Node* current = buckets[i];
            while (current) {
                keys.push_back(current->data.key);
                current = current->next;
            }
        }
        return keys;
    }
    
    size_t size() const { return size_; }
};

#endif
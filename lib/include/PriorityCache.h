#ifndef SPACEDISPLAY_PRIORITY_CACHE_H
#define SPACEDISPLAY_PRIORITY_CACHE_H

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <stdexcept>

/**
 * This cache allows to store values (of type V) by key (of type K)
 * time when key was accessed is treated as priority of this key
 * It allows to manually add values to cache or to provide supplier function
 * which will be evaluated whenever non cached value is requested
 * When trimming (reducing size of cache), keys with lowest priority (which
 * were accessed earlier then others) will be removed until size of cache is
 * reduced to desired value.
 * Trimming can be done manually or automatically (when max size is set)
 * @tparam K - type of key values, should be able to be a key for std::unordered_map
 * @tparam V - type of stored values, can be a smart pointer (both unique or shared)
 */
template<class K, class V>
class PriorityCache {
public:
    explicit PriorityCache(std::function<V(const K &)> supplier = [](const K &) -> V {
        throw std::runtime_error("Key is missing and supplier not defined");
    });

    /**
     * Get value for specific key
     * If value is not in cache, supplier function is called, result
     * put in cache and returned
     * @param key
     * @return
     */
    const V &get(const K &key);

    /**
     * Manually put value in cache
     * @param key
     * @param value
     */
    void put(const K &key, V value);

    /**
     * Change capacity of internal structures to hold N elements
     * @param N
     */
    void reserve(size_t N);

    /**
     * Check if specific key is cached
     * @param key
     * @return
     */
    bool isCached(const K &key);

    /**
     * Reduce number of elements stored to given amount
     * If N is less than current number of elements, does nothing
     * @param N
     */
    void trim(size_t N);

    /**
     * Set maximum amount of values in cache. If 0, no values will be added
     * to cache without limit
     * Cache will be trimmed after new values are added so at each time there is no more
     * than N elements in cache. Old ones (which were accessed earlier) will be removed
     * @param maxSize
     */
    void setMaxSize(size_t maxSize);

    /**
     * Removes all stored values from cache
     */
    void invalidate();

private:

    class KeyAccess {
    public:
        K key;

        /**
         * Value of access counter when this key was accessed last time
         */
        uint64_t lastAccess;

        explicit KeyAccess(K key, uint64_t lastAccess) : key(std::move(key)), lastAccess(lastAccess) {}

        friend bool operator<(const KeyAccess &a, const KeyAccess &b) {
            return a.lastAccess < b.lastAccess;
        }
    };

    class Value {
    public:
        V value;
        KeyAccess *key;

        explicit Value(V value, KeyAccess *key) : value(std::move(value)), key(key) {}
    };

    std::unordered_map<K, Value> values;
    std::vector<std::unique_ptr<KeyAccess>> keys;
    size_t maxSize = SIZE_MAX;
    uint64_t accessCounter = 0;

    std::function<V(const K &)> supplier;

    void addNewValue(const K &key, V value);
};

template<class K, class V>
PriorityCache<K, V>::PriorityCache(std::function<V(const K &)> supplier) :
        supplier(std::move(supplier)) {
}

template<class K, class V>
const V &PriorityCache<K, V>::get(const K &key) {
    auto it = values.find(key);
    if (it == values.end()) {
        addNewValue(key, std::move(supplier(key)));
        it = values.find(key);
    } else {
        it->second.key->lastAccess = accessCounter;
        ++accessCounter;
    }

    return it->second.value;
}

template<class K, class V>
void PriorityCache<K, V>::put(const K &key, V value) {
    auto it = values.find(key);
    if (it == values.end()) {
        addNewValue(key, std::move(value));
    } else {
        it->second.key->lastAccess = accessCounter;
        ++accessCounter;
        it->second.value = std::move(value);
    }
}

template<class K, class V>
void PriorityCache<K, V>::reserve(size_t N) {
    values.reserve(N);
    keys.reserve(N);
}

template<class K, class V>
bool PriorityCache<K, V>::isCached(const K &key) {
    return values.find(key) != values.end();
}

template<class K, class V>
void PriorityCache<K, V>::trim(size_t N) {
    if (keys.size() <= N)
        return;

    std::sort(keys.begin(), keys.end(), [](const auto &a, const auto &b) { return *b < *a; });

    for (size_t i = N; i < keys.size(); ++i) {
        values.erase(keys[i]->key);
    }
    keys.resize(N);
}

template<class K, class V>
void PriorityCache<K, V>::setMaxSize(size_t maxSize_) {
    if (keys.size() > maxSize_) {
        trim(maxSize_);
    }
    maxSize = maxSize_;
}

template<class K, class V>
void PriorityCache<K, V>::invalidate() {
    values.clear();
    keys.clear();
}

template<class K, class V>
void PriorityCache<K, V>::addNewValue(const K &key, V value) {
    auto *keyPtr = new KeyAccess(key, accessCounter);
    ++accessCounter;
    auto keyAccess = std::unique_ptr<KeyAccess>(keyPtr);
    keys.emplace_back(std::move(keyAccess));
    values.insert({key, std::move(Value(std::move(value), keyPtr))});

    if (maxSize > 0 && keys.size() > maxSize) {
        trim(maxSize);
    }
}

#endif //SPACEDISPLAY_PRIORITY_CACHE_H

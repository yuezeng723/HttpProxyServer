#include "cache.hpp"

template <typename K, typename V>
void LRUCache<K, V>::put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mutex);

        // If key already exists, remove it first
        auto it = map.find(key);
        if (it != map.end()) {
            cacheList.erase(it->second);
            map.erase(it);
        }

        // Add the new key and value to the front of the cache list
        cacheList.push_front(std::make_pair(key, value));
        map[key] = cacheList.begin();

        // If the cache has exceeded its capacity, remove the least recently used item
        if (map.size() > capacity) {
            auto last = cacheList.end();
            last--;
            map.erase(last->first);
            cacheList.pop_back();
        }
}

template <typename K, typename V>
std::shared_ptr<V> LRUCache<K, V>::get(const K& key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = map.find(key);
    if (it == map.end()) {
        return nullptr;
    }
    cacheList.splice(cacheList.begin(), cacheList, it->second);
    return std::make_shared<V>(it->second->second);
}

template <typename K, typename V>
void LRUCache<K, V>::remove(const K& key) {
    std::lock_guard<std::mutex> lock(mutex);

    // If key is found in the cache, remove it
    auto it = map.find(key);
    if (it != map.end()) {
        cacheList.erase(it->second);
        map.erase(it);
    }
}

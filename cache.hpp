#include <boost/shared_ptr.hpp>
#include <list>
#include <mutex>
#include <unordered_map>




template <typename K, typename V>
class LRUCache {
private:
    size_t capacity;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map;//key: requestline val:pointer to node
    std::list<std::pair<K, V>> cacheList;
    std::mutex mutex;
    
public:
    LRUCache(size_t capacity) : capacity(capacity) {}

    
    void put(const K& key, const V& value) {
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

    std::shared_ptr<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = map.find(key);
        if (it == map.end()) {
            return nullptr;
        }
        cacheList.splice(cacheList.begin(), cacheList, it->second);
        return std::make_shared<V>(it->second->second);
    }

    
    void remove(const K& key) {
        std::lock_guard<std::mutex> lock(mutex);

        // If key is found in the cache, remove it
        auto it = map.find(key);
        if (it != map.end()) {
            cacheList.erase(it->second);
            map.erase(it);
        }
    }


};

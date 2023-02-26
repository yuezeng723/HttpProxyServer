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

    void put(const K& key, const V& value);

    std::shared_ptr<V> get(const K& key);

    void remove(const K& key);

};

#include <iterator>
#include <list>
#include <thread>
#include <mutex>
#include <unordered_map>

template <typename TKey, typename TValue> class LRUCache {
private:
    class _LRUEntry {
    public:
        _LRUEntry(const TKey &key, const TValue &value) : Key(key), Value(value) { }

        TKey Key;
        TValue Value;
    };

    class _LookUpEntry {
    public:
        typename std::list<_LRUEntry>::iterator LRUEntryRef;
    };

    // Cache store cum LRU tracker
    std::list<_LRUEntry> m_lruList;

    // Key look up map
    std::unordered_map<TKey, _LookUpEntry> m_lookUpMap;

    float m_compactFactor;
    int m_capacity;
    bool m_threadSafe;

    static constexpr const float DEFAULT_COMPACT_FACTOR = 0.4;
    static constexpr const int DEFAULT_CAPACITY = 50;

    static constexpr const char *LOG_TAG = "LRUCache";

    // For conditional thread-safety support (RAII way)
    class _ConditionalMutex {
    public:
        _ConditionalMutex(bool enable) :
                m_enable(enable) { }

        void lock() { if (m_enable) m_lock.lock(); }

        void unlock() noexcept { if (m_enable) m_lock.unlock(); }

        bool try_lock() { m_enable ? m_lock.try_lock() : true; }

    private:
        std::mutex m_lock;
        bool m_enable;
    };

    _ConditionalMutex m_cacheLock;

public:
    LRUCache(int capacity = DEFAULT_CAPACITY,
             float compactionFactor = DEFAULT_COMPACT_FACTOR,
             bool threadSafe = true) : m_cacheLock(threadSafe) {
      m_capacity = capacity;
      m_compactFactor = compactionFactor;
      m_threadSafe = threadSafe;
    }

    bool Exists(const TKey &key) {
      std::lock_guard<_ConditionalMutex> guard(m_cacheLock);
      return m_lookUpMap.find(key) != m_lookUpMap.end();
    }

    TValue Get(const TKey &key) {
      std::lock_guard<_ConditionalMutex> guard(m_cacheLock);
      auto mapEntry = m_lookUpMap.find(key);
      if (mapEntry != m_lookUpMap.end()) {
        auto lruEntry = (mapEntry->second).LRUEntryRef;
        // move the item to front.
        if (lruEntry != m_lruList.begin()) {
          m_lruList.splice(m_lruList.begin(), m_lruList, lruEntry, std::next(lruEntry));
        }
        // return the copy
        return lruEntry->Value;
      } else {
        throw std::invalid_argument("key");
      }
    }

    bool Get(const TKey &key, TValue &outValue) {
      std::lock_guard<_ConditionalMutex> guard(m_cacheLock);
      auto mapEntry = m_lookUpMap.find(key);
      if (mapEntry != m_lookUpMap.end()) {
        auto lruEntry = (mapEntry->second).LRUEntryRef;
        // move the item to front.
        if (lruEntry != m_lruList.begin()) {
          m_lruList.splice(m_lruList.begin(), m_lruList, lruEntry, std::next(lruEntry));
        }
        // copy the value
        outValue = lruEntry->Value;
        return true;
      } else {
        return false;
      }
    }

    void Put(const TKey &key, const TValue &value) {
      std::lock_guard<_ConditionalMutex> guard(m_cacheLock);
      auto mapEntry = m_lookUpMap.find(key);
      if (mapEntry != m_lookUpMap.end()) {
        auto lruEntry = (mapEntry->second).LRUEntryRef;
        // move the item to front.
        if (lruEntry != m_lruList.begin()) {
          m_lruList.splice(m_lruList.begin(), m_lruList, lruEntry, std::next(lruEntry));
        }
        // copy the new value
        lruEntry->Value = value;
      } else {
        m_lruList.emplace_front(_LRUEntry(key, value));
        m_lookUpMap.emplace(key, _LookUpEntry{m_lruList.begin()});
      }
      ensureCompaction();
    }

private:
    void ensureCompaction() {
      // TODO - size() is O(n) for list, keep size internally.
      int cacheOrigSize = m_lruList.size();
      if (cacheOrigSize < m_capacity) {
        return;
      }
      int cutPosition = m_capacity * (1 - m_compactFactor);
      Log("Evicting Cache, at capacity:" + std::to_string(cacheOrigSize) + "; Cut at:" +
          to_string(cutPosition));
      // Remove entries from the look-up map
      auto lruIter = m_lruList.begin();
      for (std::advance(lruIter, cutPosition);
           lruIter != m_lruList.end();
           lruIter++) {
        m_lookUpMap.erase(lruIter->Key);
      }
      // Trim the LRU list
      {
        auto lruIter = m_lruList.begin();
        std::advance(lruIter, cutPosition);
        m_lruList.erase(lruIter, m_lruList.end());
      }
      Log("Cache trimmed to:" + std::to_string(m_lruList.size()));
    }

    void Log(const std::string &msg) {
      // TODO - it will be replaced with logging call.
      cout << LOG_TAG << ": " << msg << endl;
    }
};

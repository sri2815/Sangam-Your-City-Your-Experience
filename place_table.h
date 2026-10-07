#ifndef PLACE_TABLE_H
#define PLACE_TABLE_H

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

struct Place {
    std::string name;
    double lat = 0.0;
    double lng = 0.0;
    std::string category;
};

// Hash table with separate chaining, built from scratch for O(1) average
// place lookup by name. Case-insensitive. Resizes when load factor > 0.75.
class PlaceTable {
    struct Entry {
        std::string key;
        Place place;
        Entry *next;
    };

    std::vector<Entry *> buckets;
    size_t count = 0;

    static std::string lower(const std::string &s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return r;
    }

    // djb2 hash
    static size_t hashKey(const std::string &key) {
        size_t h = 5381;
        for (unsigned char c : key) h = ((h << 5) + h) + c;
        return h;
    }

    void rehash() {
        std::vector<Entry *> old = std::move(buckets);
        buckets.assign(old.size() * 2, nullptr);
        for (Entry *head : old) {
            while (head) {
                Entry *next = head->next;
                size_t idx = hashKey(head->key) % buckets.size();
                head->next = buckets[idx];
                buckets[idx] = head;
                head = next;
            }
        }
    }

public:
    explicit PlaceTable(size_t initialSize = 16) : buckets(initialSize, nullptr) {}

    ~PlaceTable() {
        for (Entry *head : buckets) {
            while (head) {
                Entry *next = head->next;
                delete head;
                head = next;
            }
        }
    }

    PlaceTable(const PlaceTable &) = delete;
    PlaceTable &operator=(const PlaceTable &) = delete;

    void insert(const Place &p) {
        std::string key = lower(p.name);
        size_t idx = hashKey(key) % buckets.size();
        for (Entry *e = buckets[idx]; e; e = e->next) {
            if (e->key == key) {  // update existing
                e->place = p;
                return;
            }
        }
        buckets[idx] = new Entry{key, p, buckets[idx]};
        ++count;
        if (static_cast<double>(count) / buckets.size() > 0.75) rehash();
    }

    // Returns pointer to the place, or nullptr if not found.
    const Place *find(const std::string &name) const {
        std::string key = lower(name);
        size_t idx = hashKey(key) % buckets.size();
        for (Entry *e = buckets[idx]; e; e = e->next)
            if (e->key == key) return &e->place;
        return nullptr;
    }

    std::vector<Place> all() const {
        std::vector<Place> out;
        for (Entry *head : buckets)
            for (Entry *e = head; e; e = e->next) out.push_back(e->place);
        std::sort(out.begin(), out.end(),
                  [](const Place &a, const Place &b) { return a.name < b.name; });
        return out;
    }

    size_t size() const { return count; }
};

#endif

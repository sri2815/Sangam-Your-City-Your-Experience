#ifndef TRIE_H
#define TRIE_H

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Trie for prefix-based place search / autocomplete.
// Case-insensitive: keys are lowercased on insert and lookup.
class Trie {
    struct Node {
        std::unordered_map<char, std::unique_ptr<Node>> children;
        bool isEnd = false;
        std::string original;  // original-cased name, valid when isEnd
    };

    std::unique_ptr<Node> root = std::make_unique<Node>();

    static std::string lower(const std::string &s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return r;
    }

    void collect(const Node *node, std::vector<std::string> &out, size_t limit) const {
        if (out.size() >= limit) return;
        if (node->isEnd) out.push_back(node->original);
        // std::map order isn't used, so sort keys for stable alphabetical output
        std::vector<char> keys;
        for (const auto &kv : node->children) keys.push_back(kv.first);
        std::sort(keys.begin(), keys.end());
        for (char c : keys) {
            if (out.size() >= limit) return;
            collect(node->children.at(c).get(), out, limit);
        }
    }

public:
    void insert(const std::string &name) {
        Node *cur = root.get();
        for (char c : lower(name)) {
            auto &child = cur->children[c];
            if (!child) child = std::make_unique<Node>();
            cur = child.get();
        }
        cur->isEnd = true;
        cur->original = name;
    }

    bool contains(const std::string &name) const {
        const Node *cur = root.get();
        for (char c : lower(name)) {
            auto it = cur->children.find(c);
            if (it == cur->children.end()) return false;
            cur = it->second.get();
        }
        return cur->isEnd;
    }

    // Returns up to `limit` place names starting with `prefix`.
    std::vector<std::string> autocomplete(const std::string &prefix, size_t limit = 8) const {
        std::vector<std::string> out;
        const Node *cur = root.get();
        for (char c : lower(prefix)) {
            auto it = cur->children.find(c);
            if (it == cur->children.end()) return out;
            cur = it->second.get();
        }
        collect(cur, out, limit);
        return out;
    }
};

#endif

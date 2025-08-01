#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>

namespace NAMESPACE::lsmtree {

template <typename K, typename V>
struct LRUCacheNode {
    K key;
    V value;

    LRUCacheNode* next;
};

template <typename K, typename V>
// requires std::equality_comparable<K>
class LRUCache {
    using Node = LRUCacheNode<K, V>;

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity), count_(0) {
        tail_ = new Node();
        head_ = new Node();
        head_->next = tail_;
    }

    int Get(K key, V** value) {
        auto [prev, p] = SearchKey(key);
        if (p != nullptr) {
            *value = &p->value;
            return 0;
        }
        return -1;
    }

    int Set(K key, V value) {
        auto [prev, p] = SearchKey(key);
        if (p != nullptr) {
            p->value = value;
            return 0;
        }

        // Not Found
        auto* new_node = tail_;
        if (capacity_ && count_ >= capacity_) {
            tail_ = prev;
            tail_->next = nullptr;

            // Reuse the tail_ node to avoid realloc
            *new_node = Node{
                .key = key,
                .value = value,
                .next = head_->next,
            };
        } else {
            new_node = new Node{
                .key = key,
                .value = value,
                .next = head_->next,
            };
        }

        head_->next = new_node;

        return -1;
    }

private:
    std::tuple<Node*, Node*> SearchKey(K key) {
        Node* prev = head_;
        Node* p = head_->next;
        for (; p != tail_; p = p->next) {
            if (p->key == key) {
                PromptNode(prev, p);
                return {nullptr, p};
            }
        }
        return {prev, nullptr};
    }

    void PromptNode(Node* prev, Node* p) {
        prev->next = p->next;
        p->next = head_->next;
        head_->next = p;
    }

private:
    Node* tail_;
    Node* head_;

    // Capacity 0 means no limits
    const size_t capacity_ = 0;

    size_t count_ = 0;
};
}  // namespace NAMESPACE::lsmtree

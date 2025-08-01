#pragma once

#include <concepts>

namespace NAMESPACE::lsmtree {

template <typename K, typename V>
// requires std::equality_comparable<K>
struct LinkedListNode {
    K key;
    V value;
    LinkedListNode* next;
};

template <typename K, typename V>
// requires std::equality_comparable<K>
class LinkedList {
    using Node = LinkedListNode<K, V>;

public:
    LinkedList();

    int Get(K key, V** value) {
        Node* p = head_->next;
        while (p != nullptr) {
            if (p->key == key) {
                *value = *p->value;
                return 0;
            }
            p = p->next;
        }
        return -1;
    }

    int Add(K key, V value) {
        Node* p = head_->next;

        while (p != nullptr) {
            if (p->key == key) {
                p->value = value;
                return 0;
            }
            p = p->next;
        }

        auto* new_node = new Node{
            .key = key,
            .value = value,
            .next = head_->next,
        };
        head_->next = new_node;

        return 0;
    }

private:
    Node* head_;
    Node* tail_;
};

}  // namespace NAMESPACE::lsmtree

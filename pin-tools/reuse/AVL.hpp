#ifndef PIN_REUSE_AVL_HPP
#define PIN_REUSE_AVL_HPP

#include <cstdint>
#include <functional>

template <typename T, typename Compare = std::less<T>>
class AVLTree {
  public:
    AVLTree() = default;
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;
    AVLTree(AVLTree&&) = default;
    AVLTree& operator=(AVLTree&&) = default;
    ~AVLTree() {
        destroy(root);
    }

    void insert(T key) {
        root = insertRecursive(root, key);
    }

    void erase(T key) {
        root = eraseRecursive(root, key);
    }

    bool contains(T key) const {
        return contains(root, key);
    }

    [[nodiscard]] std::size_t size() const {
        return getSize(root);
    }

    std::uint64_t getRank(T key) const {
        return getRank(root, key);
    }

  private:
    struct Node {
        T key;
        Node* left = nullptr;
        Node* right = nullptr;
        std::size_t height = 1;
        std::size_t size = 1;

        Node(T k) : key(k) {}
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&) = delete;
        Node& operator=(Node&&) = delete;
        ~Node() = default;
    };

    Node* root = nullptr;

    bool contains(Node* node, T key) const {
        if (node == nullptr) {
            return false;
        }

        if (Compare()(key, node->key)) {
            return contains(node->left, key);
        }

        if (Compare()(node->key, key)) {
            return contains(node->right, key);
        }

        return true;
    }

    // Return signed integer to allow negative heights for balance calculations.
    std::int64_t getHeight(Node* node) const {
        return node == nullptr ? 0 : node->height;
    }

    std::size_t getSize(Node* node) const {
        return node == nullptr ? 0 : node->size;
    }

    void update(Node* node) {
        if (node != nullptr) {
            node->height
                = 1 + std::max(getHeight(node->left), getHeight(node->right));
            node->size = 1 + getSize(node->left) + getSize(node->right);
        }
    }

    void destroy(Node* node) {
        if (node != nullptr) {
            destroy(node->left);
            destroy(node->right);
            delete node;
        }
    }

    std::int64_t getBalance(Node* node) const {
        return node == nullptr ? 0
                               : getHeight(node->left) - getHeight(node->right);
    }

    /* Transformation:
    **       b                  a
    **      / \                / \
    **     a   E      ->      C   b
    **    / \                    / \
    **   C   D                  D   E
    */
    Node* rotateRight(Node* b) {
        Node* a = b->left;
        b->left = a->right;
        a->right = b;

        update(b);
        update(a);

        return a;
    }

    /* Transformation:
    **     a                      b
    **    / \                    / \
    **   C   b        ->        a   E
    **      / \                / \
    **     D   E              C   D
    */
    Node* rotateLeft(Node* a) {
        Node* b = a->right;
        a->right = b->left;
        b->left = a;

        update(a);
        update(b);

        return b;
    }

    Node* insertRecursive(Node* node, T key) {
        if (node == nullptr) {
            return new Node(key);
        }

        if (Compare()(key, node->key)) {
            node->left = insertRecursive(node->left, key);
        } else if (Compare()(node->key, key)) {
            node->right = insertRecursive(node->right, key);
        } else {
            // Key already exists.
            return node;
        }

        update(node);
        const std::int64_t balance = getBalance(node);

        if (balance > 1) {
            if (Compare()(key, node->left->key)) {
                return rotateRight(node);
            }

            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }

        if (balance < -1) {
            if (Compare()(node->right->key, key)) {
                return rotateLeft(node);
            }

            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }

        return node;
    }

    Node* eraseRecursive(Node* node, T key) {
        if (node == nullptr) {
            return nullptr;
        }

        if (Compare()(key, node->key)) {
            node->left = eraseRecursive(node->left, key);
        } else if (Compare()(node->key, key)) {
            node->right = eraseRecursive(node->right, key);
        } else {
            if (node->left == nullptr || node->right == nullptr) {
                Node* replacement
                    = node->left == nullptr ? node->right : node->left;
                delete node;
                node = replacement;
            } else {
                Node* s = node->right;
                while (s->left != nullptr) {
                    s = s->left;
                }
                node->key = s->key;
                node->right = eraseRecursive(node->right, s->key);
            }
        }

        if (node == nullptr) {
            return node;
        }

        update(node);
        const std::int64_t balance = getBalance(node);

        if (balance > 1) {
            if (getBalance(node->left) >= 0) {
                return rotateRight(node);
            }

            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }

        if (balance < -1) {
            if (getBalance(node->right) <= 0) {
                return rotateLeft(node);
            }

            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }

        return node;
    }

    std::size_t getRank(Node* node, T key) const {
        if (node == nullptr) {
            return 0;
        }

        if (Compare()(key, node->key)) {
            return 1 + getSize(node->right) + getRank(node->left, key);
        }

        if (Compare()(node->key, key)) {
            return getRank(node->right, key);
        }

        return getSize(node->right);
    }
};

#endif // PIN_REUSE_AVL_HPP

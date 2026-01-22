#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Node {
    Node* next;
};

#define N 1'000
#define K 64
#define I 10'000

int main() {
    if (K < sizeof(Node*)) {
        fprintf(stderr, "K must be at least %zu bytes\n", sizeof(Node*));
        return 1;
    }

    Node* head = nullptr;
    Node* prev = nullptr;

    for (std::size_t i = 0; i < N; i++) {
        Node* node = reinterpret_cast<Node*>(malloc(K));
        memset(node, 0, K);

        if (i == 0) {
            head = node;
        } else {
            prev->next = node;
        }
        prev = node;
    }

    if (prev != nullptr) {
        prev->next = head;
    }

    Node* current = head;
    for (std::size_t i = 0; i < I; i++) {
        for (std::size_t j = 0; j < N; j++) {
            current = current->next;
        }
    }

    if (current == nullptr) {
        fprintf(stderr, "Error\n");
    }

    return 0;
}

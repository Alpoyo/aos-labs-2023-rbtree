/* Testing parameters. */
#define LEN 15
#define DO_PLOT 1
#define SEED 1337

#include <stdio.h>
#include <stdlib.h>
#if USE_AOS
#include "rbtree_aos.h"
#else
#include "rbtree.h"
#endif


#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

unsigned int BlumBlumShub(unsigned int n) {
    unsigned long long int res = n;
    for (int i = 0; i < 16; i++)
        res = res * res % 4294967291ull;
    return res;
}

struct cont {
    unsigned long val;
    struct rb_node node;
};

void print(struct rb_node *node) {
    if (!node) return;
    if (node->child[RB_LEFT]) print(node->child[RB_LEFT]);
    printf("%d ", container_of(node, struct cont, node)->val);
    fflush(stdout);
    if (node->child[RB_RIGHT]) print(node->child[RB_RIGHT]);
}

void print_dot_node(struct rb_node *node, FILE *tmp) {
    if (!node) return;

    if (node->child[RB_LEFT]) {
        fprintf(tmp, "    %llu -> %llu;\n", node, node->child[RB_LEFT]);
        print_dot_node(node->child[RB_LEFT], tmp);
    } else {
        fprintf(tmp, "    %llu -> %llu;\n", node, (unsigned long long int)node + 1);
        fprintf(tmp, "    %llu [label=\"\", width=0.1, height=0.1]\n",
                (unsigned long long int)node + 1);
    }

    fprintf(tmp, "    %llu [label=\"%d\", penwidth=5, color=%s]\n",
            node,
            container_of(node, struct cont, node)->val,
            node->color == RB_BLACK ? "black" : "red");

    if (node->child[RB_RIGHT]) {
        fprintf(tmp, "    %llu -> %llu;\n", node, node->child[RB_RIGHT]);
        print_dot_node(node->child[RB_RIGHT], tmp);
    } else {
        fprintf(tmp, "    %llu -> %llu;\n", node, (unsigned long long int)node + 2);
        fprintf(tmp, "    %llu [label=\"\", width=0.1, height=0.1]\n",
                (unsigned long long int)node + 2);
    }
}
// Filename without extension.
void rbtree_to_dot(struct rb_tree *root, char *filename) {
#if DO_PLOT
    FILE *tmp = fopen("tmp", "w");
    fprintf(tmp, "digraph RBTree {\n");
    print_dot_node(root->root, tmp);
    fprintf(tmp, "}\n");
    fclose(tmp);
    char buf[500];
    snprintf(buf, 500, "dot -Tpng %s -o %s.png", "tmp", filename);
    system(buf);
#endif
}

// new->node must be initialized beforehand.
void insert(struct rb_tree *tree, struct cont *new)
{

    struct rb_node *parent = NULL;
    struct rb_node *node2 = tree->root;
    int dir = 0;

    while (node2) {
        parent = node2;
        int cur = container_of(node2, struct cont, node)->val;
        dir = cur > new->val;
        node2 = node2->child[dir];
    }

    if (!parent) {
        tree->root = &new->node;
    } else {
        parent->child[dir] = &new->node;
        new->node.parent = parent;
    }

    rb_balance(tree, &new->node);
}

void test_rand()
{
    printf("Doing test random\n");
    char filename[500];
    int idx = 0;
    unsigned int seed = SEED;
    struct cont data[LEN] = {};
    for (int i = 0; i < LEN; i++) {
        data[i].val = i;
        rb_node_init(&data[i].node);
    }

    // Random shuffle stolen from Knuth
    for (size_t i = 0; i < LEN - 1; i++) {
        seed = BlumBlumShub(seed);
        size_t swap_idx = seed % (LEN - i - 1) + i;
        if (swap_idx == i) continue;
        data[i].val ^= data[swap_idx].val;
        data[swap_idx].val ^= data[i].val;
        data[i].val ^= data[swap_idx].val;
    }

    struct rb_tree rb = {};
    for (int i = 0; i < LEN; i++) {
        insert(&rb, data + i);
        sprintf(filename, "rand_rbtree_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }

    for (int i = 0; i < LEN; i++) {
        rb_remove(&rb, &data[i].node);
        sprintf(filename, "rand_zdeletion_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }
}

void test_sorted()
{

    printf("Doing test sorted\n");
    char filename[500];
    int idx = 0;
    unsigned int seed = SEED;
    struct cont data[LEN] = {};
    for (int i = 0; i < LEN; i++) {
        seed = BlumBlumShub(seed);
        data[i].val = i;
        rb_node_init(&data[i].node);
    }

    struct rb_tree rb = {};
    for (int i = 0; i < LEN; i++) {
        insert(&rb, data + i);
        sprintf(filename, "sorted_rbtree_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }

    for (int i = 0; i < LEN; i++) {
        rb_remove(&rb, &data[i].node);
        sprintf(filename, "sorted_zdeletion_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }
}

void test_first()
{
    printf("Doing test first\n");
    char filename[500];
    int idx = 0;
    unsigned int seed = SEED;
    struct cont data[LEN] = {};
    for (int i = 0; i < LEN; i++) {
        seed = BlumBlumShub(seed);
        data[i].val = (int)(seed & 0x0fffffffu) % (LEN * 10);
        rb_node_init(&data[i].node);
    }

    struct rb_tree rb = {};
    for (int i = 0; i < LEN; i++) {
        insert(&rb, data + i);
        sprintf(filename, "first_rbtree_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }

    for (int i = 0; i < LEN; i++) {
        struct rb_node *first = rb_first(&rb);
        printf("(step %4d)first is: %03d\n", idx, container_of(first, struct cont, node)->val);
        rb_remove(&rb, first);
        sprintf(filename, "first_zdeletion_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }
}

void test_last()
{
    printf("Doing test last\n");
    char filename[500];
    int idx = 0;
    unsigned int seed = SEED;
    struct cont data[LEN] = {};
    for (int i = 0; i < LEN; i++) {
        seed = BlumBlumShub(seed);
        data[i].val = (int)(seed & 0x0fffffffu) % (LEN * 10);
        rb_node_init(&data[i].node);
    }

    struct rb_tree rb = {};
    for (int i = 0; i < LEN; i++) {
        insert(&rb, data + i);
        sprintf(filename, "last_rbtree_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }

    for (int i = 0; i < LEN; i++) {
        struct rb_node *last = rb_last(&rb);
        printf("(step %4d)last is: %03d\n", idx, container_of(last, struct cont, node)->val);
        rb_remove(&rb, last);
        sprintf(filename, "last_zdeletion_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }
}

void test_root()
{
    printf("Doing test root\n");
    char filename[500];
    int idx = 0;
    unsigned int seed = SEED;
    struct cont data[LEN] = {};
    for (int i = 0; i < LEN; i++) {
        seed = BlumBlumShub(seed);
        data[i].val = (int)(seed & 0x0fffffffu) % (LEN * 10);
        rb_node_init(&data[i].node);
    }

    struct rb_tree rb = {};
    for (int i = 0; i < LEN; i++) {
        insert(&rb, data + i);
    }

    for (int i = 0; i < LEN; i++) {
        struct rb_node *root = rb.root;
        printf("(step %4d)root is: %03d\n", idx, container_of(root, struct cont, node)->val);
        sprintf(filename, "root_zdeletion_%03d", idx++);
        rbtree_to_dot(&rb, filename);
        rb_remove(&rb, root);
    }
}

void test_replace()
{
    printf("Doing test replace\n");
    char filename[500];
    int idx = 0;
    unsigned int seed = SEED;
    struct cont data[LEN] = {};
    struct cont data2[LEN] = {};
    for (int i = 0; i < LEN; i++) {
        seed = BlumBlumShub(seed);
        data[i].val = (int)(seed & 0x0fffffffu) % (LEN * 10);
        data2[i].val = data[i].val + LEN * 10;
        rb_node_init(&data[i].node);
        rb_node_init(&data2[i].node);
    }

    struct rb_tree rb = {};
    for (int i = 0; i < LEN; i++) {
        insert(&rb, data + i);
    }

    for (int i = 0; i < LEN; i++) {
        rb_replace(&rb, &data[i].node, &data2[i].node);
        sprintf(filename, "repl_replacement_%03d", idx++);
        rbtree_to_dot(&rb, filename);
    }

    for (int i = 0; i < LEN; i++) {
        sprintf(filename, "repl_zdeletion_%03d", idx++);
        rbtree_to_dot(&rb, filename);
        rb_remove(&rb, &data2[i].node);
    }
}

void test_iterate()
{
    printf("Doing test replace\n");
    char filename[500];
    int idx = 0;
    unsigned int seed = SEED;
    struct cont data[LEN] = {};
    struct cont data2[LEN] = {};
    for (int i = 0; i < LEN; i++) {
        seed = BlumBlumShub(seed);
        data[i].val = (int)(seed & 0x0fffffffu) % (LEN * 10);
        rb_node_init(&data[i].node);
    }

    struct rb_tree rb = {};
    for (int i = 0; i < LEN; i++) {
        insert(&rb, data + i);
    }

    sprintf(filename, "iterate_%03d", idx++);
    rbtree_to_dot(&rb, filename);

    struct rb_node *first = rb_first(&rb);
    struct rb_node *last = rb_last(&rb);
    struct rb_node *node = first;
    while (node != last) {
        printf("next val is: %03d\n", container_of(node, struct cont, node)->val);
        node = rb_next(node);
    }

    while (node != first) {
        printf("prev val is: %03d\n", container_of(node, struct cont, node)->val);
        node = rb_prev(node);
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("Usage: %s <test_index : int>\n", argv[0]);
    }
    int test_idx = strtol(argv[1], NULL, 10);

    switch (test_idx) {
        case 0: test_rand(); break;
        case 1: test_sorted(); break;
        case 2: test_first(); break;
        case 3: test_last(); break;
        case 4: test_root(); break;
        case 5: test_replace(); break;
        case 6: test_iterate(); break;
    }
    printf("Done\n");

    // TEST aos version

    return 0;
}
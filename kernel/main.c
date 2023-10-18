#include <kernel/console.h>
#include <kernel/mem.h>
#include <kernel/monitor.h>

#include <boot.h>
#include <stdio.h>
#include <string.h>
#include <rbtree.h>

#define SEED 1337
#define LEN 15

struct cont {
    unsigned long val;
    struct rb_node node;
};

unsigned int BlumBlumShub(unsigned int n) {
    unsigned long long int res = n;
    for (int i = 0; i < 16; i++)
        res = res * res % 4294967291ull;
    return res;
}

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
    }

    for (int i = 0; i < LEN; i++) {
        rb_remove(&rb, &data[i].node);
    }
}

void kmain(struct boot_info *boot_info)
{
	extern char edata[], end[];

	/* Before doing anything else, complete the ELF loading process.
	 * Clear the uninitialized global data (BSS) section of our program.
	 * This ensures that all static/global variables start out zero.
	 */
	memset(edata, 0, end - edata);

	/* Initialize the console.
	 * Can't call cprintf until after we do this! */
	cons_init();
	cprintf("\n");

	// GDB steps to get into the offending function:
	// b main.c:75
	// c
	// c
	// c
	// c
	// c
	// c
	// c
	// c
	// step
	test_rand();
	cprintf("You should see this message!\n");
}

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void _panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	/* Be extra sure that the machine is in as reasonable state */
	__asm __volatile("cli; cld");

	va_start(ap, fmt);
	cprintf("kernel panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* Break into the kernel monitor */
	while (1)
		monitor(NULL);
}

/* Like panic, but don't. */
void _warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("kernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}

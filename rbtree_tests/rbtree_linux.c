#include "rbtree.h"

// SPDX-License-Identifier: GPL-2.0-or-later
/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>
  (C) 2002  David Woodhouse <dwmw2@infradead.org>
  (C) 2012  Michel Lespinasse <walken@google.com>

  linux/lib/rbtree.c
*/

/*
 * red-black trees properties:  https://en.wikipedia.org/wiki/Rbtree
 *
 *  1) A node is either red or black
 *  2) The root is black
 *  3) All leaves (NULL) are black
 *  4) Both children of every red node are black
 *  5) Every simple path from root to leaves contains the same number
 *     of black nodes.
 *
 *  4 and 5 give the O(log n) guarantee, since 4 implies you cannot have two
 *  consecutive red nodes in a path and every red node is therefore followed by
 *  a black. So if B is the number of black nodes on every simple path (as per
 *  5), then the longest possible path due to 4 is 2B.
 *
 *  We shall indicate color with case, where black nodes are uppercase and red
 *  nodes will be lowercase. Unknown color nodes shall be drawn as red within
 *  parentheses and have some accompanying text comment.
 */

static inline void
rb_change_child(struct rb_node *old, struct rb_node *new,
                struct rb_node *parent, struct rb_tree *root)
{
    if (parent) {
        if (parent->child[RB_LEFT] == old)
            parent->child[RB_LEFT] = new;
        else
            parent->child[RB_RIGHT] = new;
    } else
        root->root = new;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new
 * - old gets assigned new as a parent and 'color' as a color.
 */
static inline void
rb_rotate_set_parents(struct rb_node *old, struct rb_node *new,
                      struct rb_tree *root, int color)
{
    struct rb_node *parent = old->parent;
    new->parent = old->parent;
    new->color = old->color;
    old->parent = new;
    old->color = color;
    rb_change_child(old, new, parent, root);
}

int rb_balance(struct rb_tree *root, struct rb_node *node)
{
    struct rb_node *parent, *gparent, *tmp;

    if (!node || !root)
        return -1;

    parent = node->parent;

    while (1) {
        /*
         * Loop invariant: node is red.
         */
        if (!parent) {
            /*
             * The inserted node is root. Either this is the
             * first node, or we recursed at Case 1 below and
             * are no longer violating 4).
             */
            node->parent = NULL;
            node->color = RB_BLACK;
            break;
        }

        /*
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as,
         * per 4), we don't want a red root or two
         * consecutive red nodes.
         */
        if(parent->color == RB_BLACK)
            break;

        gparent = parent->parent;

        tmp = gparent->child[RB_RIGHT];
        if (parent != tmp) {	/* parent == gparent->child[RB_LEFT] */
            if (tmp && tmp->color == RB_RED) {
                /*
                 * Case 1 - node's uncle is red (color flips).
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            n
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                tmp->parent = gparent;
                tmp->color = RB_BLACK;
                parent->parent = gparent;
                parent->color = RB_BLACK;
                node = gparent;
                parent = node->parent;
                node->parent = parent;
                node->color = RB_RED;
                continue;
            }

            tmp = parent->child[RB_RIGHT];
            if (node == tmp) {
                /*
                 * Case 2 - node's uncle is black and node is
                 * the parent's right child (left rotate at parent).
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                tmp = node->child[RB_LEFT];
                parent->child[RB_RIGHT] = tmp;
                node->child[RB_LEFT] = parent;
                if (tmp) {
                    tmp->parent = parent;
                    tmp->color = RB_BLACK;
                }
                parent->parent = node;
                parent->color = RB_RED;
                parent = node;
                tmp = node->child[RB_RIGHT];
            }

            /*
             * Case 3 - node's uncle is black and node is
             * the parent's left child (right rotate at gparent).
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            gparent->child[RB_LEFT] = tmp; /* == parent->child[RB_RIGHT] */
            parent->child[RB_RIGHT] = gparent;
            if (tmp) {
                tmp->parent = gparent;
                tmp->color = RB_BLACK;
            }
            rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        } else {
            tmp = gparent->child[RB_LEFT];
            if (tmp && tmp->color == RB_RED) {
                /* Case 1 - color flips */
                tmp->parent = gparent;
                tmp->color = RB_BLACK;
                parent->parent = gparent;
                parent->color = RB_BLACK;
                node = gparent;
                parent = node->parent;
                node->parent = parent;
                node->color = RB_RED;
                continue;
            }

            tmp = parent->child[RB_LEFT];
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                tmp = node->child[RB_RIGHT];
                parent->child[RB_LEFT] = tmp;
                node->child[RB_RIGHT] = parent;
                if (tmp) {
                    tmp->parent = parent;
                    tmp->color = RB_BLACK;
                }
                parent->parent = node;
                parent->color = RB_RED;
                parent = node;
                tmp = node->child[RB_LEFT];
            }

            /* Case 3 - left rotate at gparent */
            gparent->child[RB_RIGHT] = tmp; /* == parent->child[RB_LEFT] */
            parent->child[RB_LEFT] = gparent;
            if (tmp) {
                tmp->parent = gparent;
                tmp->color = RB_BLACK;
            }
            rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        }
    }

    return 0;
}

struct rb_node *rb_erase_augmented(struct rb_node *node, struct rb_tree *root)
{
    struct rb_node *child = node->child[RB_RIGHT];
    struct rb_node *tmp = node->child[RB_LEFT];
    struct rb_node *parent, *rebalance;
    struct rb_node *parent2;
    enum rb_color color;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass rb_erase_color() later on.
         */
        parent2 = node->parent;
        color = node->color;
        parent = parent2;
        rb_change_child(node, child, parent, root);
        if (child) {
            child->parent = parent2;
            child->color = color;
            rebalance = NULL;
        } else
            rebalance = color == RB_BLACK ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->child[RB_LEFT] */
        tmp->parent = parent2 = node->parent;
        tmp->color = color = node->color;
        parent = parent2;
        rb_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    } else {
        struct rb_node *successor = child, *child2;

        tmp = child->child[RB_LEFT];
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->child[RB_RIGHT];
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->child[RB_LEFT];
            } while (tmp);
            child2 = successor->child[RB_RIGHT];
            parent->child[RB_LEFT] = child2;
            successor->child[RB_RIGHT] = child;
            child->parent = successor;
        }

        tmp = node->child[RB_LEFT];
        successor->child[RB_LEFT] = tmp;
        tmp->parent = successor;

        parent2 = node->parent;
        color = node->color;
        tmp = parent2;
        rb_change_child(node, successor, tmp, root);

        if (child2) {
            child2->parent = parent;
            child2->color = RB_BLACK;
            rebalance = NULL;
        } else {
            rebalance = successor->color == RB_BLACK ? parent : NULL;
        }
        successor->parent = parent2;
        successor->color = color;
        tmp = successor;
    }

    return rebalance;
}

/*
 * Inline version for rb_erase() use - we want to be able to inline
 * and eliminate the dummy_rotate callback there
 */
void rb_erase_color(struct rb_node *parent, struct rb_tree *root)
{
    struct rb_node *node = NULL, *sibling, *tmp1, *tmp2;

    while (1) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->child[RB_RIGHT];
        if (node != sibling) {	/* node == parent->child[RB_LEFT] */
            if (sibling->color == RB_RED) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                tmp1 = sibling->child[RB_LEFT];
                parent->child[RB_RIGHT] = tmp1;
                sibling->child[RB_LEFT] = parent;
                tmp1->parent = parent;
                tmp1->color = RB_BLACK;
                rb_rotate_set_parents(parent, sibling, root,
                                      RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->child[RB_RIGHT];
            if (!tmp1 || tmp1->color == RB_BLACK) {
                tmp2 = sibling->child[RB_LEFT];
                if (!tmp2 || tmp2->color == RB_BLACK) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    sibling->parent = parent;
                    sibling->color = RB_RED;
                    if (parent->color == RB_RED)
                        parent->color = RB_BLACK;
                    else {
                        node = parent;
                        parent = node->parent;
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   sl
                 *     / \             \
                 *    sl  Sr            S
                 *                       \
                 *                        Sr
                 *
                 * Note: p might be red, and then both
                 * p and sl are red after rotation(which
                 * breaks property 4). This is fixed in
                 * Case 4 (in __rb_rotate_set_parents()
                 *         which set sl the color of p
                 *         and set p RB_BLACK)
                 *
                 *   (p)            (sl)
                 *   / \            /  \
                 *  N   sl   -->   P    S
                 *       \        /      \
                 *        S      N        Sr
                 *         \
                 *          Sr
                 */
                tmp1 = tmp2->child[RB_RIGHT];
                sibling->child[RB_LEFT] = tmp1;
                tmp2->child[RB_RIGHT] = sibling;
                parent->child[RB_RIGHT] = tmp2;
                if (tmp1) {
                    tmp1->parent = sibling;
                    tmp1->color = RB_BLACK;
                }
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */
            tmp2 = sibling->child[RB_LEFT];
            parent->child[RB_RIGHT] =  tmp2;
            sibling->child[RB_LEFT] =  parent;
            tmp1->parent = sibling;
            tmp1->color = RB_BLACK;
            if (tmp2)
                tmp2->parent = parent;
            rb_rotate_set_parents(parent, sibling, root,
                                  RB_BLACK);
            break;
        } else {
            sibling = parent->child[RB_LEFT];
            if (sibling->color == RB_RED) {
                /* Case 1 - right rotate at parent */
                tmp1 = sibling->child[RB_RIGHT];
                parent->child[RB_LEFT] =  tmp1;
                sibling->child[RB_RIGHT] =  parent;
                tmp1->parent = parent;
                tmp1->color = RB_BLACK;
                rb_rotate_set_parents(parent, sibling, root,
                                      RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->child[RB_LEFT];
            if (!tmp1 || tmp1->color == RB_BLACK) {
                tmp2 = sibling->child[RB_RIGHT];
                if (!tmp2 || tmp2->color == RB_BLACK) {
                    /* Case 2 - sibling color flip */
                    sibling->parent = parent;
                    sibling->color = RB_RED;
                    if (parent->color == RB_RED)
                        parent->color = RB_BLACK;
                    else {
                        node = parent;
                        parent = node->parent;
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - left rotate at sibling */
                tmp1 = tmp2->child[RB_LEFT];
                sibling->child[RB_RIGHT] =  tmp1;
                tmp2->child[RB_LEFT] =  sibling;
                parent->child[RB_LEFT] =  tmp2;
                if (tmp1) {
                    tmp1->parent = sibling;
                    tmp1->color = RB_BLACK;
                }
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - right rotate at parent + color flips */
            tmp2 = sibling->child[RB_RIGHT];
            parent->child[RB_LEFT] =  tmp2;
            sibling->child[RB_RIGHT] =  parent;
            tmp1->parent = sibling;
            tmp1->color = RB_BLACK;
            if (tmp2)
                tmp2->parent = parent;
            rb_rotate_set_parents(parent, sibling, root,
                                  RB_BLACK);
            break;
        }
    }
}

int rb_remove(struct rb_tree *root, struct rb_node *node)
{
    struct rb_node *rebalance;

    if (!node || !root)
        return -1;

    rebalance = rb_erase_augmented(node, root);
    if (rebalance)
        rb_erase_color(rebalance, root);

    return 0;
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
struct rb_node *rb_first(struct rb_tree *root)
{
    struct rb_node	*n;

    n = root->root;
    if (!n)
        return NULL;
    while (n->child[RB_LEFT])
        n = n->child[RB_LEFT];
    return n;
}

struct rb_node *rb_last(struct rb_tree *root)
{
    struct rb_node	*n;

    n = root->root;
    if (!n)
        return NULL;
    while (n->child[RB_RIGHT])
        n = n->child[RB_RIGHT];
    return n;
}

struct rb_node *rb_next(struct rb_node *node)
{
    struct rb_node *parent;

    if (node->parent == node)
        return NULL;

    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->child[RB_RIGHT]) {
        node = node->child[RB_RIGHT];
        while (node->child[RB_LEFT])
            node = node->child[RB_LEFT];
        return (struct rb_node *)node;
    }

    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = node->parent) && node == parent->child[RB_RIGHT])
        node = parent;

    return parent;
}

struct rb_node *rb_prev(struct rb_node *node)
{
    struct rb_node *parent;

    if (node->parent == node)
        return NULL;

    /*
     * If we have a left-hand child, go down and then right as far
     * as we can.
     */
    if (node->child[RB_LEFT]) {
        node = node->child[RB_LEFT];
        while (node->child[RB_RIGHT])
            node = node->child[RB_RIGHT];
        return (struct rb_node *)node;
    }

    /*
     * No left-hand children. Go up till we find an ancestor which
     * is a right-hand child of its parent.
     */
    while ((parent = node->parent) && node == parent->child[RB_LEFT])
        node = parent;

    return parent;
}

int rb_replace(struct rb_tree *tree, struct rb_node *node,
               struct rb_node *new_node)
{
    if (!node || !new_node)
        return -1;

    if (node->parent) {
        if (node->parent->child[RB_LEFT] == node) {
            node->parent->child[RB_LEFT] = new_node;
        } else {
            node->parent->child[RB_RIGHT] = new_node;
        }
    }
    else
        tree->root = new_node;

    if (node->child[RB_LEFT])
        node->child[RB_LEFT]->parent = new_node;

    if (node->child[RB_RIGHT])
        node->child[RB_RIGHT]->parent = new_node;

    *new_node = *node;

    return 0;
}
#ifndef LIBNC_NC_RBTREE_H_
#define LIBNC_NC_RBTREE_H_

#include <stdint.h>

#include "nc_macros.h"

#define rbtree_red(_node) ((_node)->color = 1)
#define rbtree_black(_node) ((_node)->color = 0)
#define rbtree_is_red(_node) ((_node)->color)
#define rbtree_is_black(_node) (!rbtree_is_red(_node))
#define rbtree_copy_color(_n1, _n2) ((_n1)->color = (_n2)->color)

struct nc_rbnode {
  struct nc_rbnode *left;   /* left link */
  struct nc_rbnode *right;  /* right link */
  struct nc_rbnode *parent; /* parent link */
  int64_t key;              /* key for ordering */
  void *data;               /* opaque data */
  uint8_t color;            /* red | black */
};

struct nc_rbtree {
  struct nc_rbnode *root;     /* root node */
  struct nc_rbnode *sentinel; /* nil node */
};

void nc_rbtree_node_init(struct nc_rbnode *node);
void nc_rbtree_init(struct nc_rbtree *tree, struct nc_rbnode *node);
struct nc_rbnode *nc_rbtree_min(struct nc_rbtree *tree);
void nc_rbtree_insert(struct nc_rbtree *tree, struct nc_rbnode *node);
void nc_rbtree_delete(struct nc_rbtree *tree, struct nc_rbnode *node);

#endif  // LIBNC_NC_RBTREE_H_

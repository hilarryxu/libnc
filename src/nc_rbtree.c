#include "nc_rbtree.h"

void
nc_rbtree_node_init(struct nc_rbnode *node)
{
  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;
  node->key = 0LL;
  node->data = NULL;
  /* color is left uninitialized */
}

void
nc_rbtree_init(struct nc_rbtree *tree, struct nc_rbnode *node)
{
  nc_rbtree_node_init(node);
  rbtree_black(node);
  tree->root = node;
  tree->sentinel = node;
}

static struct nc_rbnode *
rbtree_node_min(struct nc_rbnode *node, struct nc_rbnode *sentinel)
{
  /* traverse left links */

  while (node->left != sentinel) {
    node = node->left;
  }

  return node;
}

struct nc_rbnode *
nc_rbtree_min(struct nc_rbtree *tree)
{
  struct nc_rbnode *node = tree->root;
  struct nc_rbnode *sentinel = tree->sentinel;

  /* empty tree */

  if (node == sentinel) {
    return NULL;
  }

  return rbtree_node_min(node, sentinel);
}

static void
rbtree_left_rotate(struct nc_rbnode **root, struct nc_rbnode *sentinel,
                   struct nc_rbnode *node)
{
  struct nc_rbnode *temp;

  temp = node->right;
  node->right = temp->left;

  if (temp->left != sentinel) {
    temp->left->parent = node;
  }

  temp->parent = node->parent;

  if (node == *root) {
    *root = temp;
  } else if (node == node->parent->left) {
    node->parent->left = temp;
  } else {
    node->parent->right = temp;
  }

  temp->left = node;
  node->parent = temp;
}

static void
rbtree_right_rotate(struct nc_rbnode **root, struct nc_rbnode *sentinel,
                    struct nc_rbnode *node)
{
  struct nc_rbnode *temp;

  temp = node->left;
  node->left = temp->right;

  if (temp->right != sentinel) {
    temp->right->parent = node;
  }

  temp->parent = node->parent;

  if (node == *root) {
    *root = temp;
  } else if (node == node->parent->right) {
    node->parent->right = temp;
  } else {
    node->parent->left = temp;
  }

  temp->right = node;
  node->parent = temp;
}

void
nc_rbtree_insert(struct nc_rbtree *tree, struct nc_rbnode *node)
{
  struct nc_rbnode **root = &tree->root;
  struct nc_rbnode *sentinel = tree->sentinel;
  struct nc_rbnode *temp, **p;

  /* empty tree */

  if (*root == sentinel) {
    node->parent = NULL;
    node->left = sentinel;
    node->right = sentinel;
    rbtree_black(node);
    *root = node;
    return;
  }

  /* a binary tree insert */

  temp = *root;
  for (;;) {
    p = (node->key < temp->key) ? &temp->left : &temp->right;
    if (*p == sentinel) {
      break;
    }
    temp = *p;
  }

  *p = node;
  node->parent = temp;
  node->left = sentinel;
  node->right = sentinel;
  rbtree_red(node);

  /* re-balance tree */

  while (node != *root && rbtree_is_red(node->parent)) {
    if (node->parent == node->parent->parent->left) {
      temp = node->parent->parent->right;

      if (rbtree_is_red(temp)) {
        rbtree_black(node->parent);
        rbtree_black(temp);
        rbtree_red(node->parent->parent);
        node = node->parent->parent;
      } else {
        if (node == node->parent->right) {
          node = node->parent;
          rbtree_left_rotate(root, sentinel, node);
        }

        rbtree_black(node->parent);
        rbtree_red(node->parent->parent);
        rbtree_right_rotate(root, sentinel, node->parent->parent);
      }
    } else {
      temp = node->parent->parent->left;

      if (rbtree_is_red(temp)) {
        rbtree_black(node->parent);
        rbtree_black(temp);
        rbtree_red(node->parent->parent);
        node = node->parent->parent;
      } else {
        if (node == node->parent->left) {
          node = node->parent;
          rbtree_right_rotate(root, sentinel, node);
        }

        rbtree_black(node->parent);
        rbtree_red(node->parent->parent);
        rbtree_left_rotate(root, sentinel, node->parent->parent);
      }
    }
  }

  rbtree_black(*root);
}

void
nc_rbtree_delete(struct nc_rbtree *tree, struct nc_rbnode *node)
{
  struct nc_rbnode **root = &tree->root;
  struct nc_rbnode *sentinel = tree->sentinel;
  struct nc_rbnode *subst, *temp, *w;
  uint8_t red;

  /* a binary tree delete */

  if (node->left == sentinel) {
    temp = node->right;
    subst = node;
  } else if (node->right == sentinel) {
    temp = node->left;
    subst = node;
  } else {
    subst = rbtree_node_min(node->right, sentinel);
    temp = subst->right;
  }

  if (subst == *root) {
    *root = temp;
    rbtree_black(temp);

    nc_rbtree_node_init(node);

    return;
  }

  red = rbtree_is_red(subst);

  if (subst == subst->parent->left) {
    subst->parent->left = temp;
  } else {
    subst->parent->right = temp;
  }

  if (subst == node) {
    temp->parent = subst->parent;
  } else {
    if (subst->parent == node) {
      temp->parent = subst;
    } else {
      temp->parent = subst->parent;
    }

    subst->left = node->left;
    subst->right = node->right;
    subst->parent = node->parent;
    rbtree_copy_color(subst, node);

    if (node == *root) {
      *root = subst;
    } else {
      if (node == node->parent->left) {
        node->parent->left = subst;
      } else {
        node->parent->right = subst;
      }
    }

    if (subst->left != sentinel) {
      subst->left->parent = subst;
    }

    if (subst->right != sentinel) {
      subst->right->parent = subst;
    }
  }

  nc_rbtree_node_init(node);

  if (red) {
    return;
  }

  /* a delete fixup */

  while (temp != *root && rbtree_is_black(temp)) {
    if (temp == temp->parent->left) {
      w = temp->parent->right;

      if (rbtree_is_red(w)) {
        rbtree_black(w);
        rbtree_red(temp->parent);
        rbtree_left_rotate(root, sentinel, temp->parent);
        w = temp->parent->right;
      }

      if (rbtree_is_black(w->left) && rbtree_is_black(w->right)) {
        rbtree_red(w);
        temp = temp->parent;
      } else {
        if (rbtree_is_black(w->right)) {
          rbtree_black(w->left);
          rbtree_red(w);
          rbtree_right_rotate(root, sentinel, w);
          w = temp->parent->right;
        }

        rbtree_copy_color(w, temp->parent);
        rbtree_black(temp->parent);
        rbtree_black(w->right);
        rbtree_left_rotate(root, sentinel, temp->parent);
        temp = *root;
      }

    } else {
      w = temp->parent->left;

      if (rbtree_is_red(w)) {
        rbtree_black(w);
        rbtree_red(temp->parent);
        rbtree_right_rotate(root, sentinel, temp->parent);
        w = temp->parent->left;
      }

      if (rbtree_is_black(w->left) && rbtree_is_black(w->right)) {
        rbtree_red(w);
        temp = temp->parent;
      } else {
        if (rbtree_is_black(w->left)) {
          rbtree_black(w->right);
          rbtree_red(w);
          rbtree_left_rotate(root, sentinel, w);
          w = temp->parent->left;
        }

        rbtree_copy_color(w, temp->parent);
        rbtree_black(temp->parent);
        rbtree_black(w->left);
        rbtree_right_rotate(root, sentinel, temp->parent);
        temp = *root;
      }
    }
  }

  rbtree_black(temp);
}

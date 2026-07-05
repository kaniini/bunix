#ifndef BUNIXOS_TREE_H
#define BUNIXOS_TREE_H

#include "types.h"

struct tree_node {
	struct tree_node *left;
	struct tree_node *right;
	struct tree_node *parent;
	const char *key;
	u64 value;
};

struct tree {
	struct tree_node *root;
	u64 count;
};

struct u64_tree_node {
	struct u64_tree_node *left;
	struct u64_tree_node *right;
	struct u64_tree_node *parent;
	u64 key;
	u64 value;
};

struct u64_tree {
	struct u64_tree_node *root;
	u64 count;
};

static inline void tree_init(struct tree *tree)
{
	tree->root = 0;
	tree->count = 0;
}

static inline void u64_tree_init(struct u64_tree *tree)
{
	tree->root = 0;
	tree->count = 0;
}

static inline int tree_key_cmp(const char *left, const char *right)
{
	u64 i = 0;

	while (left[i] != '\0' && right[i] != '\0') {
		if (left[i] < right[i]) {
			return -1;
		}
		if (left[i] > right[i]) {
			return 1;
		}
		i++;
	}
	if (left[i] == right[i]) {
		return 0;
	}
	return left[i] == '\0' ? -1 : 1;
}

static inline struct tree_node *tree_find_node(const struct tree *tree,
					       const char *key)
{
	struct tree_node *node;

	if (tree == 0 || key == 0) {
		return 0;
	}
	node = tree->root;
	while (node != 0) {
		const int cmp = tree_key_cmp(key, node->key);

		if (cmp == 0) {
			return node;
		}
		node = cmp < 0 ? node->left : node->right;
	}
	return 0;
}

static inline u64 tree_get(const struct tree *tree, const char *key)
{
	const struct tree_node *node = tree_find_node(tree, key);

	return node != 0 ? node->value : 0;
}

static inline int tree_insert_node(struct tree *tree,
				   struct tree_node *node,
				   const char *key, u64 value)
{
	struct tree_node *parent = 0;
	struct tree_node **slot;
	int cmp = 0;

	if (tree == 0 || node == 0 || key == 0 || value == 0) {
		return -1;
	}
	slot = &tree->root;
	while (*slot != 0) {
		parent = *slot;
		cmp = tree_key_cmp(key, parent->key);
		if (cmp == 0) {
			return -1;
		}
		slot = cmp < 0 ? &parent->left : &parent->right;
	}
	node->left = 0;
	node->right = 0;
	node->parent = parent;
	node->key = key;
	node->value = value;
	*slot = node;
	tree->count++;
	return 0;
}

static inline void tree_transplant(struct tree *tree,
				   struct tree_node *old_node,
				   struct tree_node *new_node)
{
	if (old_node->parent == 0) {
		tree->root = new_node;
	} else if (old_node == old_node->parent->left) {
		old_node->parent->left = new_node;
	} else {
		old_node->parent->right = new_node;
	}
	if (new_node != 0) {
		new_node->parent = old_node->parent;
	}
}

static inline void tree_remove_node(struct tree *tree, struct tree_node *node)
{
	struct tree_node *next;

	if (tree == 0 || node == 0) {
		return;
	}
	if (node->left == 0) {
		tree_transplant(tree, node, node->right);
	} else if (node->right == 0) {
		tree_transplant(tree, node, node->left);
	} else {
		next = node->right;
		while (next->left != 0) {
			next = next->left;
		}
		if (next->parent != node) {
			tree_transplant(tree, next, next->right);
			next->right = node->right;
			next->right->parent = next;
		}
		tree_transplant(tree, node, next);
		next->left = node->left;
		next->left->parent = next;
	}
	node->left = 0;
	node->right = 0;
	node->parent = 0;
	node->key = 0;
	node->value = 0;
	if (tree->count != 0) {
		tree->count--;
	}
}

static inline struct u64_tree_node *u64_tree_find_node(
	const struct u64_tree *tree, u64 key)
{
	struct u64_tree_node *node;

	if (tree == 0 || key == 0) {
		return 0;
	}
	node = tree->root;
	while (node != 0) {
		if (key == node->key) {
			return node;
		}
		node = key < node->key ? node->left : node->right;
	}
	return 0;
}

static inline u64 u64_tree_get(const struct u64_tree *tree, u64 key)
{
	const struct u64_tree_node *node = u64_tree_find_node(tree, key);

	return node != 0 ? node->value : 0;
}

static inline struct u64_tree_node *u64_tree_first_node(
	const struct u64_tree *tree)
{
	struct u64_tree_node *node;

	if (tree == 0) {
		return 0;
	}
	node = tree->root;
	if (node == 0) {
		return 0;
	}
	while (node->left != 0) {
		node = node->left;
	}
	return node;
}

static inline struct u64_tree_node *u64_tree_next_node(
	const struct u64_tree_node *node)
{
	const struct u64_tree_node *current = node;
	struct u64_tree_node *next;

	if (node == 0) {
		return 0;
	}
	if (node->right != 0) {
		next = node->right;
		while (next->left != 0) {
			next = next->left;
		}
		return next;
	}
	while (current->parent != 0 && current == current->parent->right) {
		current = current->parent;
	}
	return current->parent;
}

static inline int u64_tree_insert_node(struct u64_tree *tree,
				       struct u64_tree_node *node,
				       u64 key, u64 value)
{
	struct u64_tree_node *parent = 0;
	struct u64_tree_node **slot;

	if (tree == 0 || node == 0 || key == 0 || value == 0) {
		return -1;
	}
	slot = &tree->root;
	while (*slot != 0) {
		parent = *slot;
		if (key == parent->key) {
			return -1;
		}
		slot = key < parent->key ? &parent->left : &parent->right;
	}
	node->left = 0;
	node->right = 0;
	node->parent = parent;
	node->key = key;
	node->value = value;
	*slot = node;
	tree->count++;
	return 0;
}

static inline void u64_tree_transplant(struct u64_tree *tree,
				       struct u64_tree_node *old_node,
				       struct u64_tree_node *new_node)
{
	if (old_node->parent == 0) {
		tree->root = new_node;
	} else if (old_node == old_node->parent->left) {
		old_node->parent->left = new_node;
	} else {
		old_node->parent->right = new_node;
	}
	if (new_node != 0) {
		new_node->parent = old_node->parent;
	}
}

static inline void u64_tree_remove_node(struct u64_tree *tree,
					struct u64_tree_node *node)
{
	struct u64_tree_node *next;

	if (tree == 0 || node == 0) {
		return;
	}
	if (node->left == 0) {
		u64_tree_transplant(tree, node, node->right);
	} else if (node->right == 0) {
		u64_tree_transplant(tree, node, node->left);
	} else {
		next = node->right;
		while (next->left != 0) {
			next = next->left;
		}
		if (next->parent != node) {
			u64_tree_transplant(tree, next, next->right);
			next->right = node->right;
			next->right->parent = next;
		}
		u64_tree_transplant(tree, node, next);
		next->left = node->left;
		next->left->parent = next;
	}
	node->left = 0;
	node->right = 0;
	node->parent = 0;
	node->key = 0;
	node->value = 0;
	if (tree->count != 0) {
		tree->count--;
	}
}

#endif

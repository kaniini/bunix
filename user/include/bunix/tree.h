#ifndef BUNIX_USER_TREE_H
#define BUNIX_USER_TREE_H

#include <bunix/syscall.h>

struct bunix_tree_node {
	struct bunix_tree_node *left;
	struct bunix_tree_node *right;
	struct bunix_tree_node *parent;
	const char *key;
	u64 value;
};

struct bunix_tree {
	struct bunix_tree_node *root;
	u64 count;
};

struct bunix_u64_tree_node {
	struct bunix_u64_tree_node *left;
	struct bunix_u64_tree_node *right;
	struct bunix_u64_tree_node *parent;
	u64 key;
	u64 value;
};

struct bunix_u64_tree {
	struct bunix_u64_tree_node *root;
	u64 count;
};

static inline void bunix_tree_init(struct bunix_tree *tree)
{
	tree->root = 0;
	tree->count = 0;
}

static inline void bunix_u64_tree_init(struct bunix_u64_tree *tree)
{
	tree->root = 0;
	tree->count = 0;
}

static inline int bunix_tree_key_cmp(const char *left, const char *right)
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

static inline struct bunix_tree_node *bunix_tree_find_node(
	const struct bunix_tree *tree, const char *key)
{
	struct bunix_tree_node *node;

	if (tree == 0 || key == 0) {
		return 0;
	}
	node = tree->root;
	while (node != 0) {
		const int cmp = bunix_tree_key_cmp(key, node->key);

		if (cmp == 0) {
			return node;
		}
		node = cmp < 0 ? node->left : node->right;
	}
	return 0;
}

static inline u64 bunix_tree_get(const struct bunix_tree *tree,
				 const char *key)
{
	const struct bunix_tree_node *node = bunix_tree_find_node(tree, key);

	return node != 0 ? node->value : 0;
}

static inline int bunix_tree_insert_node(struct bunix_tree *tree,
					 struct bunix_tree_node *node,
					 const char *key, u64 value)
{
	struct bunix_tree_node *parent = 0;
	struct bunix_tree_node **slot;
	int cmp = 0;

	if (tree == 0 || node == 0 || key == 0 || value == 0) {
		return -1;
	}
	slot = &tree->root;
	while (*slot != 0) {
		parent = *slot;
		cmp = bunix_tree_key_cmp(key, parent->key);
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

static inline struct bunix_tree_node *bunix_tree_first_node(
	const struct bunix_tree *tree)
{
	struct bunix_tree_node *node;

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

static inline struct bunix_tree_node *bunix_tree_next_node(
	const struct bunix_tree_node *node)
{
	const struct bunix_tree_node *current = node;
	struct bunix_tree_node *next;

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

static inline void bunix_tree_transplant(struct bunix_tree *tree,
					 struct bunix_tree_node *old_node,
					 struct bunix_tree_node *new_node)
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

static inline void bunix_tree_remove_node(struct bunix_tree *tree,
					  struct bunix_tree_node *node)
{
	struct bunix_tree_node *next;

	if (tree == 0 || node == 0) {
		return;
	}
	if (node->left == 0) {
		bunix_tree_transplant(tree, node, node->right);
	} else if (node->right == 0) {
		bunix_tree_transplant(tree, node, node->left);
	} else {
		next = node->right;
		while (next->left != 0) {
			next = next->left;
		}
		if (next->parent != node) {
			bunix_tree_transplant(tree, next, next->right);
			next->right = node->right;
			next->right->parent = next;
		}
		bunix_tree_transplant(tree, node, next);
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

static inline struct bunix_u64_tree_node *bunix_u64_tree_find_node(
	const struct bunix_u64_tree *tree, u64 key)
{
	struct bunix_u64_tree_node *node;

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

static inline u64 bunix_u64_tree_get(const struct bunix_u64_tree *tree,
				     u64 key)
{
	const struct bunix_u64_tree_node *node =
		bunix_u64_tree_find_node(tree, key);

	return node != 0 ? node->value : 0;
}

static inline struct bunix_u64_tree_node *bunix_u64_tree_first_node(
	const struct bunix_u64_tree *tree)
{
	struct bunix_u64_tree_node *node;

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

static inline struct bunix_u64_tree_node *bunix_u64_tree_next_node(
	const struct bunix_u64_tree_node *node)
{
	const struct bunix_u64_tree_node *current = node;
	struct bunix_u64_tree_node *next;

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

static inline int bunix_u64_tree_insert_node(
	struct bunix_u64_tree *tree, struct bunix_u64_tree_node *node,
	u64 key, u64 value)
{
	struct bunix_u64_tree_node *parent = 0;
	struct bunix_u64_tree_node **slot;

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

static inline void bunix_u64_tree_transplant(
	struct bunix_u64_tree *tree, struct bunix_u64_tree_node *old_node,
	struct bunix_u64_tree_node *new_node)
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

static inline void bunix_u64_tree_remove_node(
	struct bunix_u64_tree *tree, struct bunix_u64_tree_node *node)
{
	struct bunix_u64_tree_node *next;

	if (tree == 0 || node == 0) {
		return;
	}
	if (node->left == 0) {
		bunix_u64_tree_transplant(tree, node, node->right);
	} else if (node->right == 0) {
		bunix_u64_tree_transplant(tree, node, node->left);
	} else {
		next = node->right;
		while (next->left != 0) {
			next = next->left;
		}
		if (next->parent != node) {
			bunix_u64_tree_transplant(tree, next, next->right);
			next->right = node->right;
			next->right->parent = next;
		}
		bunix_u64_tree_transplant(tree, node, next);
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

/*
 * Copyright (c) 2009-2011 Petri Lehtinen <petri@digip.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef LIBNC_NC_HASHTABLE_H_
#define LIBNC_NC_HASHTABLE_H_

#include <stddef.h>  // size_t

typedef size_t (*nc_hashtable_key_hash_pt)(const void *key);
typedef int (*nc_hashtable_key_cmp_pt)(const void *key1, const void *key2);
typedef void (*nc_hashtable_free_pt)(void *key);

struct hashtable_list {
  struct hashtable_list *prev;
  struct hashtable_list *next;
};

struct hashtable_pair {
  void *key;
  void *value;
  size_t hash;
  struct hashtable_list list;
};

struct hashtable_bucket {
  struct hashtable_list *first;
  struct hashtable_list *last;
};

struct nc_hashtable {
  size_t size;
  struct hashtable_bucket *buckets;
  size_t num_buckets; /* index to primes[] */
  struct hashtable_list list;

  nc_hashtable_key_hash_pt hash_key;
  nc_hashtable_key_cmp_pt cmp_keys; /* returns non-zero for equal keys */
  nc_hashtable_free_pt free_key;
  nc_hashtable_free_pt free_value;
};

/**
 * nc_hashtable_create - Create a hashtable object
 *
 * @hash_key: The key hashing function
 * @cmp_keys: The key compare function. Returns non-zero for equal and
 *     zero for unequal unequal keys
 * @free_key: If non-NULL, called for a key that is no longer referenced.
 * @free_value: If non-NULL, called for a value that is no longer referenced.
 *
 * Returns a new hashtable object that should be freed with
 * nc_hashtable_destroy when it's no longer used, or NULL on failure (out
 * of memory).
 */
struct nc_hashtable *nc_hashtable_create(nc_hashtable_key_hash_pt hash_key,
                                         nc_hashtable_key_cmp_pt cmp_keys,
                                         nc_hashtable_free_pt free_key,
                                         nc_hashtable_free_pt free_value);

/**
 * nc_hashtable_destroy - Destroy a hashtable object
 *
 * @hashtable: The hashtable
 *
 * Destroys a hashtable created with hashtable_create().
 */
void nc_hashtable_destroy(struct nc_hashtable *hashtable);

/**
 * nc_hashtable_init - Initialize a hashtable object
 *
 * @hashtable: The (statically allocated) hashtable object
 * @hash_key: The key hashing function
 * @cmp_keys: The key compare function. Returns non-zero for equal and
 *     zero for unequal unequal keys
 * @free_key: If non-NULL, called for a key that is no longer referenced.
 * @free_value: If non-NULL, called for a value that is no longer referenced.
 *
 * Initializes a statically allocated hashtable object. The object
 * should be cleared with nc_hashtable_deinit when it's no longer used.
 *
 * Returns 0 on success, -1 on error (out of memory).
 */
int nc_hashtable_init(struct nc_hashtable *hashtable,
                      nc_hashtable_key_hash_pt hash_key,
                      nc_hashtable_key_cmp_pt cmp_keys,
                      nc_hashtable_free_pt free_key,
                      nc_hashtable_free_pt free_value);

/**
 * nc_hashtable_deinit - Release all resources used by a hashtable object
 *
 * @hashtable: The hashtable
 *
 * Destroys a statically allocated hashtable object.
 */
void nc_hashtable_deinit(struct nc_hashtable *hashtable);

/**
 * nc_hashtable_set - Add/modify value in hashtable
 *
 * @hashtable: The hashtable object
 * @key: The key
 * @value: The value
 *
 * If a value with the given key already exists, its value is replaced
 * with the new value.
 *
 * Key and value are "stealed" in the sense that hashtable frees them
 * automatically when they are no longer used. The freeing is
 * accomplished by calling free_key and free_value functions that were
 * supplied to hashtable_new. In case one or both of the free
 * functions is NULL, the corresponding item is not "stealed".
 *
 * Returns 0 on success, -1 on failure (out of memory).
 */
int nc_hashtable_set(struct nc_hashtable *hashtable, void *key, void *value);

/**
 * nc_hashtable_get - Get a value associated with a key
 *
 * @hashtable: The hashtable object
 * @key: The key
 *
 * Returns value if it is found, or NULL otherwise.
 */
void *nc_hashtable_get(struct nc_hashtable *hashtable, const void *key);

/**
 * nc_hashtable_del - Remove a value from the hashtable
 *
 * @hashtable: The hashtable object
 * @key: The key
 *
 * Returns 0 on success, or -1 if the key was not found.
 */
int nc_hashtable_del(struct nc_hashtable *hashtable, const void *key);

/**
 * nc_hashtable_clear - Clear hashtable
 *
 * @hashtable: The hashtable object
 *
 * Removes all items from the hashtable.
 */
void nc_hashtable_clear(struct nc_hashtable *hashtable);

/**
 * nc_hashtable_iter - Iterate over hashtable
 *
 * @hashtable: The hashtable object
 *
 * Returns an opaque iterator to the first element in the hashtable.
 * The iterator should be passed to hashtable_iter_* functions.
 * The hashtable items are not iterated over in any particular order.
 *
 * There's no need to free the iterator in any way. The iterator is
 * valid as long as the item that is referenced by the iterator is not
 * deleted. Other values may be added or deleted. In particular,
 * hashtable_iter_next() may be called on an iterator, and after that
 * the key/value pair pointed by the old iterator may be deleted.
 */
void *nc_hashtable_iter(struct nc_hashtable *hashtable);

/**
 * nc_hashtable_iter_at - Return an iterator at a specific key
 *
 * @hashtable: The hashtable object
 * @key: The key that the iterator should point to
 *
 * Like nc_hashtable_iter() but returns an iterator pointing to a
 * specific key.
 */
void *nc_hashtable_iter_at(struct nc_hashtable *hashtable, const void *key);

/**
 * nc_hashtable_iter_next - Advance an iterator
 *
 * @hashtable: The hashtable object
 * @iter: The iterator
 *
 * Returns a new iterator pointing to the next element in the
 * hashtable or NULL if the whole hastable has been iterated over.
 */
void *nc_hashtable_iter_next(struct nc_hashtable *hashtable, void *iter);

/**
 * nc_hashtable_iter_key - Retrieve the key pointed by an iterator
 *
 * @iter: The iterator
 */
void *nc_hashtable_iter_key(void *iter);

/**
 * nc_hashtable_iter_value - Retrieve the value pointed by an iterator
 *
 * @iter: The iterator
 */
void *nc_hashtable_iter_value(void *iter);

/**
 * nc_hashtable_iter_set - Set the value pointed by an iterator
 *
 * @iter: The iterator
 * @value: The value to set
 */
void nc_hashtable_iter_set(struct nc_hashtable *hashtable, void *iter,
                           void *value);

#endif  // LIBNC_NC_HASHTABLE_H_

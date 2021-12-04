#include "nc_hashtable.h"

#include <stdlib.h>

#include "nc_macros.h"

typedef struct hashtable_list list_t;
typedef struct hashtable_pair pair_t;
typedef struct hashtable_bucket bucket_t;

#define container_of(ptr_, type_, member_) \
  ((type_ *)((char *)ptr_ - offsetof(type_, member_)))

#define list_to_pair(list_) container_of(list_, pair_t, list)

static inline void
list_init(list_t *list)
{
  list->next = list;
  list->prev = list;
}

static inline void
list_insert(list_t *list, list_t *node)
{
  node->next = list;
  node->prev = list->prev;
  list->prev->next = node;
  list->prev = node;
}

static inline void
list_remove(list_t *list)
{
  list->prev->next = list->next;
  list->next->prev = list->prev;
}

static inline int
bucket_is_empty(struct nc_hashtable *hashtable, bucket_t *bucket)
{
  return bucket->first == &hashtable->list && bucket->first == bucket->last;
}

static void
insert_to_bucket(struct nc_hashtable *hashtable, bucket_t *bucket, list_t *list)
{
  if (bucket_is_empty(hashtable, bucket)) {
    list_insert(&hashtable->list, list);
    bucket->first = bucket->last = list;
  } else {
    list_insert(bucket->first, list);
    bucket->first = list;
  }
}

static size_t primes[] = {5,         13,        23,        53,        97,
                          193,       389,       769,       1543,      3079,
                          6151,      12289,     24593,     49157,     98317,
                          196613,    393241,    786433,    1572869,   3145739,
                          6291469,   12582917,  25165843,  50331653,  100663319,
                          201326611, 402653189, 805306457, 1610612741};

static inline size_t
num_buckets(struct nc_hashtable *hashtable)
{
  return primes[hashtable->num_buckets];
}

static pair_t *
hashtable_find_pair(struct nc_hashtable *hashtable, bucket_t *bucket,
                    const void *key, size_t hash)
{
  list_t *list;
  pair_t *pair;

  if (bucket_is_empty(hashtable, bucket))
    return NULL;

  list = bucket->first;
  while (1) {
    pair = list_to_pair(list);
    if (pair->hash == hash && hashtable->cmp_keys(pair->key, key))
      return pair;

    if (list == bucket->last)
      break;

    list = list->next;
  }

  return NULL;
}

/* returns 0 on success, -1 if key was not found */
static int
hashtable_do_del(struct nc_hashtable *hashtable, const void *key, size_t hash)
{
  pair_t *pair;
  bucket_t *bucket;
  size_t index;

  index = hash % num_buckets(hashtable);
  bucket = &hashtable->buckets[index];

  pair = hashtable_find_pair(hashtable, bucket, key, hash);
  if (!pair)
    return -1;

  if (&pair->list == bucket->first && &pair->list == bucket->last)
    bucket->first = bucket->last = &hashtable->list;

  else if (&pair->list == bucket->first)
    bucket->first = pair->list.next;

  else if (&pair->list == bucket->last)
    bucket->last = pair->list.prev;

  list_remove(&pair->list);

  if (hashtable->free_key)
    hashtable->free_key(pair->key);
  if (hashtable->free_value)
    hashtable->free_value(pair->value);

  nc_free(pair);
  hashtable->size--;

  return 0;
}

static void
hashtable_do_clear(struct nc_hashtable *hashtable)
{
  list_t *list, *next;
  pair_t *pair;

  for (list = hashtable->list.next; list != &hashtable->list; list = next) {
    next = list->next;
    pair = list_to_pair(list);
    if (hashtable->free_key)
      hashtable->free_key(pair->key);
    if (hashtable->free_value)
      hashtable->free_value(pair->value);
    nc_free(pair);
  }
}

static int
hashtable_do_rehash(struct nc_hashtable *hashtable)
{
  list_t *list, *next;
  pair_t *pair;
  size_t i, index, new_size;

  nc_free(hashtable->buckets);

  hashtable->num_buckets++;
  new_size = num_buckets(hashtable);

  hashtable->buckets = nc_alloc(new_size * sizeof(bucket_t));
  if (!hashtable->buckets)
    return -1;

  for (i = 0; i < num_buckets(hashtable); i++) {
    hashtable->buckets[i].first = hashtable->buckets[i].last = &hashtable->list;
  }

  list = hashtable->list.next;
  list_init(&hashtable->list);

  for (; list != &hashtable->list; list = next) {
    next = list->next;
    pair = list_to_pair(list);
    index = pair->hash % new_size;
    insert_to_bucket(hashtable, &hashtable->buckets[index], &pair->list);
  }

  return 0;
}

struct nc_hashtable *
nc_hashtable_create(nc_hashtable_key_hash_pt hash_key,
                    nc_hashtable_key_cmp_pt cmp_keys,
                    nc_hashtable_free_pt free_key,
                    nc_hashtable_free_pt free_value)
{
  struct nc_hashtable *hashtable = nc_alloc(sizeof(struct nc_hashtable));
  if (!hashtable)
    return NULL;

  if (nc_hashtable_init(hashtable, hash_key, cmp_keys, free_key, free_value)) {
    nc_free(hashtable);
    return NULL;
  }

  return hashtable;
}

void
nc_hashtable_destroy(struct nc_hashtable *hashtable)
{
  nc_hashtable_deinit(hashtable);
  nc_free(hashtable);
}

int
nc_hashtable_init(struct nc_hashtable *hashtable,
                  nc_hashtable_key_hash_pt hash_key,
                  nc_hashtable_key_cmp_pt cmp_keys,
                  nc_hashtable_free_pt free_key,
                  nc_hashtable_free_pt free_value)
{
  size_t i;

  hashtable->size = 0;
  hashtable->num_buckets = 0; /* index to primes[] */
  hashtable->buckets = nc_alloc(num_buckets(hashtable) * sizeof(bucket_t));
  if (!hashtable->buckets)
    return -1;

  list_init(&hashtable->list);

  hashtable->hash_key = hash_key;
  hashtable->cmp_keys = cmp_keys;
  hashtable->free_key = free_key;
  hashtable->free_value = free_value;

  for (i = 0; i < num_buckets(hashtable); i++) {
    hashtable->buckets[i].first = hashtable->buckets[i].last = &hashtable->list;
  }

  return 0;
}

void
nc_hashtable_deinit(struct nc_hashtable *hashtable)
{
  hashtable_do_clear(hashtable);
  nc_free(hashtable->buckets);
}

int
nc_hashtable_set(struct nc_hashtable *hashtable, void *key, void *value)
{
  pair_t *pair;
  bucket_t *bucket;
  size_t hash, index;

  /* rehash if the load ratio exceeds 1 */
  if (hashtable->size >= num_buckets(hashtable))
    if (hashtable_do_rehash(hashtable))
      return -1;

  hash = hashtable->hash_key(key);
  index = hash % num_buckets(hashtable);
  bucket = &hashtable->buckets[index];
  pair = hashtable_find_pair(hashtable, bucket, key, hash);

  if (pair) {
    if (hashtable->free_key)
      hashtable->free_key(key);
    if (hashtable->free_value)
      hashtable->free_value(pair->value);
    pair->value = value;
  } else {
    pair = nc_alloc(sizeof(pair_t));
    if (!pair)
      return -1;

    pair->key = key;
    pair->value = value;
    pair->hash = hash;
    list_init(&pair->list);

    insert_to_bucket(hashtable, bucket, &pair->list);

    hashtable->size++;
  }
  return 0;
}

void *
nc_hashtable_get(struct nc_hashtable *hashtable, const void *key)
{
  pair_t *pair;
  size_t hash;
  bucket_t *bucket;

  hash = hashtable->hash_key(key);
  bucket = &hashtable->buckets[hash % num_buckets(hashtable)];

  pair = hashtable_find_pair(hashtable, bucket, key, hash);
  if (!pair)
    return NULL;

  return pair->value;
}

int
nc_hashtable_del(struct nc_hashtable *hashtable, const void *key)
{
  size_t hash = hashtable->hash_key(key);
  return hashtable_do_del(hashtable, key, hash);
}

void
nc_hashtable_clear(struct nc_hashtable *hashtable)
{
  size_t i;

  hashtable_do_clear(hashtable);

  for (i = 0; i < num_buckets(hashtable); i++) {
    hashtable->buckets[i].first = hashtable->buckets[i].last = &hashtable->list;
  }

  list_init(&hashtable->list);
  hashtable->size = 0;
}

void *
nc_hashtable_iter(struct nc_hashtable *hashtable)
{
  return nc_hashtable_iter_next(hashtable, &hashtable->list);
}

void *
nc_hashtable_iter_at(struct nc_hashtable *hashtable, const void *key)
{
  pair_t *pair;
  size_t hash;
  bucket_t *bucket;

  hash = hashtable->hash_key(key);
  bucket = &hashtable->buckets[hash % num_buckets(hashtable)];

  pair = hashtable_find_pair(hashtable, bucket, key, hash);
  if (!pair)
    return NULL;

  return &pair->list;
}

void *
nc_hashtable_iter_next(struct nc_hashtable *hashtable, void *iter)
{
  list_t *list = (list_t *)iter;
  if (list->next == &hashtable->list)
    return NULL;
  return list->next;
}

void *
nc_hashtable_iter_key(void *iter)
{
  pair_t *pair = list_to_pair((list_t *)iter);
  return pair->key;
}

void *
nc_hashtable_iter_value(void *iter)
{
  pair_t *pair = list_to_pair((list_t *)iter);
  return pair->value;
}

void
nc_hashtable_iter_set(struct nc_hashtable *hashtable, void *iter, void *value)
{
  pair_t *pair = list_to_pair((list_t *)iter);

  if (hashtable->free_value)
    hashtable->free_value(pair->value);

  pair->value = value;
}

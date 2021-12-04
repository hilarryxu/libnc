#ifndef LIBNC_NC_PALLOC_H_
#define LIBNC_NC_PALLOC_H_

#include <stdint.h>

#include "nc_macros.h"

struct nc_pool;
struct nc_pool_large;

#define NC_MAX_ALLOC_FROM_POOL (4096 - 1)

#define NC_DEFAULT_POOL_SIZE (16 * 1024)

#define NC_POOL_ALIGNMENT 16
#define NC_MIN_POOL_SIZE                                                \
  nc_align((sizeof(struct nc_pool) + 2 * sizeof(struct nc_pool_large)), \
           NC_POOL_ALIGNMENT)

struct nc_pool_large {
  struct nc_pool_large *next;
  void *alloc;
};

typedef void (*nc_pool_cleanup_pt)(void *data);

struct nc_pool_cleanup {
  nc_pool_cleanup_pt handler;
  void *data;
  struct nc_pool_cleanup *next;
};

struct nc_pool_data {
  u_char *last;
  u_char *end;
  struct nc_pool *next;
  uint32_t failed;
};

struct nc_pool {
  struct nc_pool_data d;
  size_t max;
  struct nc_pool *current;
  struct nc_pool_large *large;
  struct nc_pool_cleanup *cleanup;
};

struct nc_pool *nc_pool_create(size_t size);
void nc_pool_destroy(struct nc_pool *pool);
void nc_pool_reset(struct nc_pool *pool);

void *nc_palloc(struct nc_pool *pool, size_t size);
void *nc_pnalloc(struct nc_pool *pool, size_t size);
void *nc_pcalloc(struct nc_pool *pool, size_t size);
void *nc_pmemalign(struct nc_pool *pool, size_t size, size_t alignment);
int nc_pfree(struct nc_pool *pool, void *p);

struct nc_pool_cleanup *nc_pool_cleanup_add(struct nc_pool *p, size_t size);

#endif  // LIBNC_NC_PALLOC_H_

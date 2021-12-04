#include "nc_palloc.h"

#include <string.h>  // memset

static inline void *nc_palloc_small(struct nc_pool *pool, size_t size,
                                    int align);
static void *nc_palloc_block(struct nc_pool *pool, size_t size);
static void *nc_palloc_large(struct nc_pool *pool, size_t size);

struct nc_pool *
nc_pool_create(size_t size)
{
  struct nc_pool *p;

  p = nc_memalign(NC_POOL_ALIGNMENT, size);
  if (p == NULL) {
    return NULL;
  }

  p->d.last = (u_char *)p + sizeof(struct nc_pool);
  p->d.end = (u_char *)p + size;
  p->d.next = NULL;
  p->d.failed = 0;

  size = size - sizeof(struct nc_pool);
  p->max = MIN(size, NC_MAX_ALLOC_FROM_POOL);

  p->current = p;
  p->large = NULL;
  p->cleanup = NULL;

  return p;
}

void
nc_pool_destroy(struct nc_pool *pool)
{
  struct nc_pool *p, *n;
  struct nc_pool_large *l;
  struct nc_pool_cleanup *c;

  // Run cleanup handlers
  for (c = pool->cleanup; c; c = c->next) {
    if (c->handler) {
      log_debug(LOG_VVERB, "run cleanup: %p", c);
      c->handler(c->data);
    }
  }

  // Free large memory blocks
  for (l = pool->large; l; l = l->next) {
    if (l->alloc) {
      nc_free(l->alloc);
    }
  }

  // Free pools
  for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
    nc_free(p);

    if (n == NULL) {
      break;
    }
  }
}

void
nc_pool_reset(struct nc_pool *pool)
{
  struct nc_pool *p;
  struct nc_pool_large *l;

  // Free large memory blocks
  for (l = pool->large; l; l = l->next) {
    if (l->alloc) {
      nc_free(l->alloc);
    }
  }

  // Rset pool's last idx
  for (p = pool; p; p = p->d.next) {
    // FIXME(xcc): special handle first pool and other pools
    p->d.last = (u_char *)p + sizeof(struct nc_pool);
    p->d.failed = 0;
  }

  pool->current = pool;
  pool->large = NULL;
}

int
nc_pfree(struct nc_pool *pool, void *p)
{
  struct nc_pool_large *l;

  // Search p in large memory blocks, and then free it.
  for (l = pool->large; l; l = l->next) {
    if (p == l->alloc) {
      log_debug(LOG_VVERB, "free: %p", l->alloc);
      nc_free(l->alloc);
      l->alloc = NULL;

      return NC_OK;
    }
  }

  return NC_ERROR;
}

void *
nc_palloc(struct nc_pool *pool, size_t size)
{
#if !(NC_DEBUG_PALLOC)
  if (size <= pool->max) {
    return nc_palloc_small(pool, size, 1);
  }
#endif

  return nc_palloc_large(pool, size);
}

void *
nc_pnalloc(struct nc_pool *pool, size_t size)
{
#if !(NC_DEBUG_PALLOC)
  if (size <= pool->max) {
    return nc_palloc_small(pool, size, 0);
  }
#endif

  return nc_palloc_large(pool, size);
}

void *
nc_pcalloc(struct nc_pool *pool, size_t size)
{
  void *p;

  p = nc_palloc(pool, size);
  if (p) {
    memset(p, 0, size);
  }

  return p;
}

static inline void *
nc_palloc_small(struct nc_pool *pool, size_t size, int align)
{
  u_char *m;
  struct nc_pool *p;

  // First search pool(ensure must not NULL)
  p = pool->current;

  do {
    m = p->d.last;

    if (align) {
      m = NC_ALIGN_PTR(m, NC_ALIGNMENT);
    }

    // Left space is enough
    if ((size_t)(p->d.end - m) >= size) {
      p->d.last = m + size;

      return m;
    }

    // Search next pool
    p = p->d.next;

  } while (p);

  // No pool meet it, alloc a new pool
  return nc_palloc_block(pool, size);
}

static void *
nc_palloc_block(struct nc_pool *pool, size_t size)
{
  u_char *m;
  size_t psize;
  struct nc_pool *p, *new_p;

  // Calc pool size
  psize = (size_t)(pool->d.end - (u_char *)pool);

  m = nc_memalign(NC_POOL_ALIGNMENT, psize);
  if (m == NULL) {
    return NULL;
  }

  new_p = (struct nc_pool *)m;

  new_p->d.end = m + psize;
  new_p->d.next = NULL;
  new_p->d.failed = 0;

  // Not first pool
  // So use struct nc_pool_data
  m += sizeof(struct nc_pool_data);
  m = NC_ALIGN_PTR(m, NC_ALIGNMENT);
  // Set d.last idx
  new_p->d.last = m + size;

  // Iter pools, set p to last one
  for (p = pool->current; p->d.next; p = p->d.next) {
    if (p->d.failed++ > 4) {
      // pool->current unchanged
      // or fisrt pool not NULL and p->d.failed < 5
      pool->current = p->d.next;
    }
  }

  // Link new pool to last one pool
  p->d.next = new_p;

  return m;
}

static void *
nc_palloc_large(struct nc_pool *pool, size_t size)
{
  void *p;
  int n;
  struct nc_pool_large *large;

  p = nc_alloc(size);
  if (p == NULL) {
    return NULL;
  }

  // Check first 3 large items if can reuse
  n = 0;
  for (large = pool->large; large; large = large->next) {
    if (large->alloc == NULL) {
      large->alloc = p;
      return p;
    }

    if (n++ > 3) {
      break;
    }
  }

  // Align alloc struct nc_pool_large in pool
  large = nc_palloc_small(pool, sizeof(struct nc_pool_large), 1);
  if (large == NULL) {
    nc_free(p);
    return NULL;
  }

  // Link it to head list
  large->alloc = p;
  large->next = pool->large;
  pool->large = large;

  return p;
}

struct nc_pool_cleanup *
nc_pool_cleanup_add(struct nc_pool *p, size_t size)
{
  struct nc_pool_cleanup *c;

  c = nc_palloc(p, sizeof(struct nc_pool_cleanup));
  if (c == NULL) {
    return NULL;
  }

  if (size) {
    c->data = nc_palloc(p, size);
    if (c->data == NULL) {
      return NULL;
    }

  } else {
    c->data = NULL;
  }

  c->handler = NULL;
  c->next = p->cleanup;

  p->cleanup = c;

  log_debug(LOG_VVERB, "add cleanup: %p", c);

  return c;
}

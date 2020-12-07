#ifndef LIBNC_NC_ARRAY_H_
#define LIBNC_NC_ARRAY_H_

#include "nc_macros.h"

struct nc_array {
  void *elems;
  int nelem;
  size_t size;
  int nalloc;
};

struct nc_array *nc_array_create(int n, size_t size);
void nc_array_destroy(struct nc_array *a);
void *nc_array_push(struct nc_array *a);
void *nc_array_push_n(struct nc_array *a, int n);

static inline int
nc_array_init(struct nc_array *array, int n, size_t size)
{
  ASSERT(n != 0 && size != 0);

  array->elems = nc_alloc(n * size);
  if (array->elems == NULL) {
    return NC_ENOMEM;
  }

  array->nelem = 0;
  array->size = size;
  array->nalloc = n;

  return NC_OK;
}

#endif // LIBNC_NC_ARRAY_H_

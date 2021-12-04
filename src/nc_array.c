#include "nc_array.h"

#include <stdint.h>

struct nc_array *
nc_array_create(int n, size_t size)
{
  struct nc_array *a;

  NC_ASSERT(n != 0 && size != 0);

  a = nc_alloc(sizeof(*a));
  if (a == NULL) {
    return NULL;
  }

  if (nc_array_init(a, n, size) != NC_OK) {
    nc_free(a);
    return NULL;
  }

  return a;
}

void
nc_array_destroy(struct nc_array *a)
{
  if (a->elems != NULL) {
    nc_free(a->elems);
  }
  nc_free(a);
}

void *
nc_array_push(struct nc_array *a)
{
  void *elem, *new_elem;
  size_t size;

  if (a->nelem == a->nalloc) {
    size = a->size * a->nalloc;
    new_elem = nc_realloc(a->elems, 2 * size);
    if (new_elem == NULL) {
      return NULL;
    }

    a->elems = new_elem;
    a->nalloc *= 2;
  }

  elem = (u_char *)a->elems + a->size * a->nelem;
  a->nelem++;

  return elem;
}

void *
nc_array_push_n(struct nc_array *a, int n)
{
  void *elem, *new_elem;
  size_t size;
  int nalloc;

  if (a->nelem + n > a->nalloc) {
    nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);
    new_elem = nc_realloc(a->elems, nalloc * a->size);
    if (new_elem == NULL) {
      return NULL;
    }

    a->elems = new_elem;
    a->nalloc = nalloc;
  }

  elem = (u_char *)a->elems + a->size * a->nelem;
  a->nelem += n;

  return elem;
}

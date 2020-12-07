#include "nc_macros.h"

#include <stdlib.h>
#include <string.h>

void *
_nc_alloc(size_t size, const char *name, int line)
{
  void *p;

  ASSERT(size != 0);

  p = malloc(size);
  if (p == NULL) {
    log_error("malloc(%zu) failed @ %s:%d", size, name, line);
  } else {
    log_debug(LOG_VVERB, "malloc(%zu) at %p @ %s:%d", size, p, name, line);
  }

  return p;
}

void *
_nc_zalloc(size_t size, const char *name, int line)
{
  void *p;

  p = _nc_alloc(size, name, line);
  if (p != NULL) {
    memset(p, 0, size);
  }

  return p;
}

void *
_nc_calloc(size_t nmemb, size_t size, const char *name, int line)
{
  return _nc_zalloc(nmemb * size, name, line);
}

void *
_nc_realloc(void *ptr, size_t size, const char *name, int line)
{
  void *p;

  ASSERT(size != 0);

  p = realloc(ptr, size);
  if (p == NULL) {
    log_error("realloc(%zu) failed @ %s:%d", size, name, line);
  } else {
    log_debug(LOG_VVERB, "realloc(%zu) at %p @ %s:%d", size, p, name, line);
  }

  return p;
}

void
_nc_free(void *ptr, const char *name, int line)
{
  ASSERT(ptr != NULL);
  log_debug(LOG_VVERB, "free(%p) @ %s:%d", ptr, name, line);
  free(ptr);
}

void
nc_assert(const char *cond, const char *file, int line, int panic)
{
  log_error("assert '%s' failed @ (%s, %d)", cond, file, line);
  if (panic) {
    // nc_stacktrace(1);
    abort();
  }
}

#if (NC_HAVE_POSIX_MEMALIGN)

void *
nc_memalign(size_t alignment, size_t size)
{
  void *p;
  int err;

  err = posix_memalign(&p, alignment, size);

  if (err) {
    log_error("posix_memalign(%uz, %uz) failed", alignment, size);
    p = NULL;
  }

  log_debug(LOG_VVERB, "posix_memalign: %p:%uz @%uz", p, size, alignment);

  return p;
}

#endif

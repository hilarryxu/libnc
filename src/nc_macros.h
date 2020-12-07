#ifndef LIBNC_NC_MACROS_H_
#define LIBNC_NC_MACROS_H_

#include <stddef.h> // size_t

typedef unsigned char u_char;

#define LF (uint8_t)10
#define CR (uint8_t)13
#define CRLF "\x0d\x0a"
#define CRLF_LEN (sizeof("\x0d\x0a") - 1)

#define NELEMS(a) ((sizeof(a)) / sizeof((a)[0]))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// From stdint.h, we have:
// # define UINT8_MAX	(255)
// # define UINT16_MAX	(65535)
// # define UINT32_MAX	(4294967295U)
// # define UINT64_MAX	(__UINT64_C(18446744073709551615))
#define NC_UINT8_MAXLEN (3 + 1)
#define NC_UINT16_MAXLEN (5 + 1)
#define NC_UINT32_MAXLEN (10 + 1)
#define NC_UINT64_MAXLEN (20 + 1)
#define NC_UINTMAX_MAXLEN NC_UINT64_MAXLEN

// Make data 'd' or pointer 'p', n-byte aligned, where n is a power of 2.
#define NC_ALIGNMENT sizeof(unsigned long) /* platform word */
#define NC_ALIGN(d, n) (((d) + (n - 1)) & ~(n - 1))
#define NC_ALIGN_PTR(p, n)                                                     \
  (void *)(((uintptr_t)(p) + ((uintptr_t)n - 1)) & ~((uintptr_t)n - 1))

#define NC_OK 0
#define NC_ERROR -1
#define NC_EAGAIN -2
#define NC_ENOMEM -3

//
// memory alloc
//
#define nc_alloc(_s) _nc_alloc((size_t)(_s), __FILE__, __LINE__)

#define nc_zalloc(_s) _nc_zalloc((size_t)(_s), __FILE__, __LINE__)

#define nc_calloc(_n, _s)                                                      \
  _nc_calloc((size_t)(_n), (size_t)(_s), __FILE__, __LINE__)

#define nc_realloc(_p, _s) _nc_realloc(_p, (size_t)(_s), __FILE__, __LINE__)

#define nc_free(_p)                                                            \
  do {                                                                         \
    _nc_free(_p, __FILE__, __LINE__);                                          \
    (_p) = NULL;                                                               \
  } while (0)

void *_nc_alloc(size_t size, const char *name, int line);
void *_nc_zalloc(size_t size, const char *name, int line);
void *_nc_calloc(size_t nmemb, size_t size, const char *name, int line);
void *_nc_realloc(void *ptr, size_t size, const char *name, int line);
void _nc_free(void *ptr, const char *name, int line);

#if (NC_HAVE_POSIX_MEMALIGN)

void *nc_memalign(size_t alignment, size_t size);

#else

#define nc_memalign(alignment, size) nc_alloc(size)

#endif

// log_stderr   - log to stderr
// loga         - log always
// loga_hexdump - log hexdump always
// log_error    - error log messages
// log_warn     - warning log messages
// log_panic    - log messages followed by a panic
// ...
// log_debug    - debug log messages based on a log level
// log_hexdump  - hexadump -C of a log buffer
#define log_debug(_level, ...)
#define log_error(...)
#define log_warn(...)
#define log_panic(...)

//
// assert
//
#ifdef NC_ASSERT_PANIC

#define ASSERT(_x)                                                             \
  do {                                                                         \
    if (!(_x)) {                                                               \
      nc_assert(#_x, __FILE__, __LINE__, 1);                                   \
    }                                                                          \
  } while (0)

#define NOT_REACHED() ASSERT(0)

#elif NC_ASSERT_LOG

#define ASSERT(_x)                                                             \
  do {                                                                         \
    if (!(_x)) {                                                               \
      nc_assert(#_x, __FILE__, __LINE__, 0);                                   \
    }                                                                          \
  } while (0)

#define NOT_REACHED() ASSERT(0)

#else

#define ASSERT(_x)

#define NOT_REACHED()

#endif // ASSERT

void nc_assert(const char *cond, const char *file, int line, int panic);

#endif // LIBNC_NC_MACROS_H_

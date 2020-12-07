#ifndef LIBNC_NC_MBUF_H_
#define LIBNC_NC_MBUF_H_

#include <stdint.h>
#include <stddef.h>

#include "nc_macros.h"
#include "nc_sys_queue.h"

struct nc_mbuf {
  uint32_t magic;             // mbuf magic (const)
  STAILQ_ENTRY(nc_mbuf) next; // next mbuf
  u_char *pos;                // read marker
  u_char *last;               // write marker
  u_char *start;              // start of buffer (const)
  u_char *end;                // end of buffer (const)
};

STAILQ_HEAD(nc_mhdr, nc_mbuf);

struct nc_mbuf_head {
  struct nc_mhdr mhdr;
  size_t chunk_size;
};

typedef void (*nc_mbuf_copy_pt)(struct nc_mbuf *, void *);

#define NC_MBUF_MAGIC 0xdeadbeef
#define NC_MBUF_HSIZE sizeof(struct nc_mbuf)

static inline int
nc_mbuf_empty(struct nc_mbuf *mbuf)
{
  return mbuf->pos == mbuf->last ? 1 : 0;
}

static inline int
nc_mbuf_full(struct nc_mbuf *mbuf)
{
  return mbuf->last == mbuf->end ? 1 : 0;
}

int nc_mbuf_init_head(struct nc_mbuf_head *head, size_t chunk_size);
void nc_mbuf_deinit_head(struct nc_mbuf_head *head);

// Insert mbuf to tail
void nc_mbuf_insert(struct nc_mbuf_head *head, struct nc_mbuf *mbuf);
// Remove mbuf from mhdr
void nc_mbuf_remove(struct nc_mbuf_head *head, struct nc_mbuf *mbuf);

// Alloc a new mbuf or reuse from free buf list
struct nc_mbuf *nc_mbuf_get(struct nc_mbuf_head *head, size_t chunk_size);
// Put mbuf back into free buf list head
void nc_mbuf_put(struct nc_mbuf_head *head, struct nc_mbuf *mbuf);

void nc_mbuf_rewind(struct nc_mbuf *mbuf);

static inline size_t
nc_mbuf_len(struct nc_mbuf *mbuf)
{
  ASSERT(mbuf->last >= mbuf->pos);

  return (size_t)(mbuf->last - mbuf->pos);
}
static inline size_t
nc_mbuf_freespace(struct nc_mbuf *mbuf)
{
  ASSERT(mbuf->end >= mbuf->last);

  return (size_t)(mbuf->end - mbuf->last);
}

static inline size_t
nc_mbuf_capacity(struct nc_mbuf *mbuf)
{
  return (size_t)(mbuf->end - mbuf->start);
}

// Append data to mbuf
void nc_mbuf_memcopy(struct nc_mbuf *mbuf, u_char *pos, size_t n);
// Split some tail data from last mbuf to a new mbuf
struct nc_mbuf *mbuf_split(struct nc_mbuf_head *head, uint8_t *pos,
                           nc_mbuf_copy_pt cb, void *cb_arg);

#endif // LIBNC_NC_MBUF_H_

#ifndef LIBNC_NC_MBUF_H_
#define LIBNC_NC_MBUF_H_

#include <stdint.h>
#include <stddef.h>

#include "nc_macros.h"
#include "nc_sys_queue.h"

struct nc_mbuf {
  uint32_t magic;             /* mbuf magic (const) */
  STAILQ_ENTRY(nc_mbuf) next; /* next mbuf */
  u_char *pos;                /* read marker */
  u_char *last;               /* write marker */
  u_char *start;              /* start of buffer (const) */
  u_char *end;                /* end of buffer (const) */
};

STAILQ_HEAD(nc_mhdr, nc_mbuf);

struct nc_mbuf_inst {
  struct nc_mhdr head;
  size_t size;
};

#define MBUF_MAGIC 0xdeadbeef
#define MBUF_MIN_SIZE 512
#define MBUF_MAX_SIZE 16777216
#define MBUF_SIZE 16384
#define MBUF_HSIZE sizeof(struct mbuf)

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

int nc_mbuf_init(struct nc_mbuf_inst *head);

void nc_mbuf_remove(struct nc_mbuf_inst *inst, struct nc_mbuf *mbuf);

#endif // LIBNC_NC_MBUF_H_

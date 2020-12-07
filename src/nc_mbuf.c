#include "nc_mbuf.h"

static void
nc_mbuf_free(struct nc_mbuf *mbuf)
{
  u_char *buf;

  log_debug(LOG_VVERB, "put mbuf %p len %d", mbuf, mbuf->last - mbuf->pos);

  ASSERT(STAILQ_NEXT(mbuf, next) == NULL);
  ASSERT(mbuf->magic == MBUF_MAGIC);

  buf = (u_char *)mbuf - /*mbuf_offset*/ 0;
  nc_free(buf);
}

int
nc_mbuf_init(struct nc_mbuf_inst *inst)
{
  struct nc_mhdr *head = (struct nc_mhdr *)inst;
  STAILQ_INIT(head);
  inst->size = 0;
  return 0;
}

void
nc_mbuf_deinit(struct nc_mbuf_inst *inst)
{
  struct nc_mhdr *head = (struct nc_mhdr *)inst;
  while (!STAILQ_EMPTY(head)) {
    struct nc_mbuf *mbuf = STAILQ_FIRST(head);
    nc_mbuf_remove(inst, mbuf);
    nc_mbuf_free(mbuf);
    inst->size--;
  }
  ASSERT(inst->size == 0);
}

void
nc_mbuf_remove(struct nc_mbuf_inst *inst, struct nc_mbuf *mbuf)
{
  log_debug(LOG_VVERB, "remove mbuf %p len %d", mbuf, mbuf->last - mbuf->pos);
  struct nc_mhdr *head = (struct nc_mhdr *)inst;
  STAILQ_REMOVE(head, mbuf, nc_mbuf, next);
  STAILQ_NEXT(mbuf, next) = NULL;
}

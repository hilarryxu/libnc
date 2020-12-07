#include "nc_mbuf.h"

#include <string.h>

/*
 * mbuf header is at the tail end of the mbuf. This enables us to catch
 * buffer overrun early by asserting on the magic value during get or
 * put operations
 *
 *   <------------- mbuf_chunk_size ------------->
 *   +-------------------------------------------+
 *   |       mbuf data          |  mbuf header   |
 *   |     (mbuf_offset)        | (struct mbuf)  |
 *   +-------------------------------------------+
 *   ^           ^        ^     ^^
 *   |           |        |     ||
 *   \           |        |     |\
 *   mbuf->start \        |     | mbuf->end (one byte past valid bound)
 *                mbuf->pos     \
 *                        \      mbuf
 *                        mbuf->last (one byte past valid byte)
 *
 */

static void
free_mbuf(struct nc_mbuf *mbuf)
{
  log_debug(LOG_VVERB, "put mbuf %p len %d", mbuf, mbuf->last - mbuf->pos);

  ASSERT(STAILQ_NEXT(mbuf, next) == NULL);
  ASSERT(mbuf->magic == MBUF_MAGIC);

  nc_free(mbuf->start);
}

int
nc_mbuf_init_head(struct nc_mbuf_head *head, size_t chunk_size)
{
  struct nc_mhdr *mhdr = (struct nc_mhdr *)head;
  STAILQ_INIT(mhdr);
  head->chunk_size = chunk_size;
  return 0;
}

void
nc_mbuf_deinit_head(struct nc_mbuf_head *head)
{
  struct nc_mbuf *mbuf;

  struct nc_mhdr *mhdr = (struct nc_mhdr *)head;
  while (!STAILQ_EMPTY(mhdr)) {
    mbuf = STAILQ_FIRST(mhdr);
    nc_mbuf_remove(head, mbuf);
    free_mbuf(mbuf);
  }
}

void
nc_mbuf_insert(struct nc_mbuf_head *head, struct nc_mbuf *mbuf)
{
  struct nc_mhdr *mhdr = (struct nc_mhdr *)head;
  STAILQ_INSERT_TAIL(mhdr, mbuf, next);
  log_debug(LOG_VVERB, "insert mbuf %p len %d", mbuf, mbuf->last - mbuf->pos);
}

void
nc_mbuf_remove(struct nc_mbuf_head *head, struct nc_mbuf *mbuf)
{
  log_debug(LOG_VVERB, "remove mbuf %p len %d", mbuf, mbuf->last - mbuf->pos);
  struct nc_mhdr *mhdr = (struct nc_mhdr *)head;
  STAILQ_REMOVE(mhdr, mbuf, nc_mbuf, next);
  STAILQ_NEXT(mbuf, next) = NULL;
}

struct nc_mbuf *
nc_mbuf_get(struct nc_mbuf_head *head, size_t chunk_size)
{
  struct nc_mbuf *mbuf;
  u_char *buf;
  size_t mbuf_offset = chunk_size - NC_MBUF_HSIZE;

  if (head) {
    ASSERT(head->chunk_size == chunk_size);
    struct nc_mhdr *mhdr = (struct nc_mhdr *)head;
    if (!STAILQ_EMPTY(mhdr)) {
      mbuf = STAILQ_FIRST(mhdr);
      STAILQ_REMOVE_HEAD(mhdr, next);

      ASSERT(mbuf->magic == MBUF_MAGIC);
      goto done;
    }
  }

  buf = nc_alloc(chunk_size);
  if (buf == NULL) {
    return NULL;
  }
  mbuf = (struct nc_mbuf *)(buf + mbuf_offset);
  mbuf->magic = NC_MBUF_MAGIC;

done:
  STAILQ_NEXT(mbuf, next) = NULL;
  mbuf->start = buf;
  mbuf->end = buf + mbuf_offset;
  ASSERT(mbuf->end - mbuf->start == (int)mbuf_offset);
  ASSERT(mbuf->start < mbuf->end);
  mbuf->pos = mbuf->start;
  mbuf->last = mbuf->start;

  log_debug(LOG_VVERB, "get mbuf %p", mbuf);

  return mbuf;
}

void
nc_mbuf_put(struct nc_mbuf_head *head, struct nc_mbuf *mbuf)
{
  ASSERT(head != NULL);
  log_debug(LOG_VVERB, "put mbuf %p len %d", mbuf, mbuf->last - mbuf->pos);

  ASSERT(STAILQ_NEXT(mbuf, next) == NULL);
  ASSERT(mbuf->magic == MBUF_MAGIC);

  struct nc_mhdr *mhdr = (struct nc_mhdr *)head;
  STAILQ_INSERT_HEAD(mhdr, mbuf, next);
}

void
nc_mbuf_rewind(struct nc_mbuf *mbuf)
{
  mbuf->pos = mbuf->start;
  mbuf->last = mbuf->start;
}

void
nc_mbuf_memcopy(struct nc_mbuf *mbuf, u_char *pos, size_t n)
{
  if (n == 0) {
    return;
  }

  ASSERT(!nc_mbuf_full(mbuf) && n <= nc_mbuf_freespace(mbuf));

  ASSERT(pos < mbuf->start || pos >= mbuf->end);

  memcpy(mbuf->last, pos, n);
  mbuf->last += n;
}

struct nc_mbuf *
mbuf_split(struct nc_mbuf_head *head, uint8_t *pos, nc_mbuf_copy_pt cb,
           void *cb_arg)
{
  struct nc_mbuf *mbuf, *nbuf;
  size_t size;

  struct nc_mhdr *mhdr = (struct nc_mhdr *)head;
  ASSERT(!STAILQ_EMPTY(mhdr));

  mbuf = STAILQ_LAST(mhdr, nc_mbuf, next);
  ASSERT(pos >= mbuf->pos && pos <= mbuf->last);

  nbuf = nc_mbuf_get(NULL, head->chunk_size);
  if (nbuf == NULL) {
    return NULL;
  }

  if (cb != NULL) {
    // Precopy nbuf
    cb(nbuf, cb_arg);
  }

  // Copy data from mbuf to nbuf
  size = (size_t)(mbuf->last - pos);
  nc_mbuf_memcopy(nbuf, pos, size);

  // Adjust mbuf
  mbuf->last = pos;

  return nbuf;
}

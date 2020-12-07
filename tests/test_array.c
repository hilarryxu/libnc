#include "nc_array.h"
#include "greatest.h"

struct t_pos {
  int x;
  int y;
};

TEST basic(void) {
  struct t_pos *p;
  int i;

  struct nc_array *arr = nc_array_create(5, sizeof(struct t_pos));
  for (i = 1; i <= 3; i++) {
    p = (struct t_pos *)nc_array_push(arr);
    p->x = i;
    p->y = i;
  }
  ASSERT_EQ(3, arr->nelem);

  p = (struct t_pos *)nc_array_push_n(arr, 4);
  for (i = 0; i < 4; i++) {
    p[i].x = i + 4;
    p[i].y = i + 4;
  }
  ASSERT_EQ(7, arr->nelem);

  p = (struct t_pos *)arr->elems;
  for (i = 0; i < arr->nelem; i++) {
    // printf(" [%d] (%d, %d)\n", i + 1, p[i].x, p[i].y);
    ASSERT_EQ(i + 1, p[i].x);
    ASSERT_EQ(i + 1, p[i].y);
  }
  ASSERT_EQ(10, arr->nalloc);

  nc_array_destroy(arr);
  PASS();
}

SUITE(array) {
  RUN_TEST(basic);
}

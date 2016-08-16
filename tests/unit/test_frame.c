#include "unit.h"

#include <newt/common.h>
#include <newt/frame.h>
#include <newt/stomp.h>

#include <string.h>

static void validate_frame(frame_t *frame, int expect_headers, int expect_bodylen) {
  struct list_head *e;
  linedata_t *body;
  int headers = 0;
  int bodylen = 0;

  // validation for frame headers
  list_for_each(e, &frame->h_attrs) {
    headers++;
  }
  CU_ASSERT(headers == expect_headers);

  // validation for frame body
  CU_ASSERT_FATAL(! list_empty(&frame->h_data));
  list_for_each_entry(body, &frame->h_data, list) {
    bodylen += body->len;
  }
  CU_ASSERT(bodylen == expect_bodylen);
}

static void check_alloc() {
  frame_t *frame = alloc_frame();

  CU_ASSERT(frame != NULL);

  free_frame(frame);
}

static void check_parse() {
  frame_t *frame;
  char raw_frame[] = "SEND\n"
    "content-length:24\n"
    "some-additional-header:hoge\n"
    "\n"
    "(body) hoge\n"
    "(body) fuga\n";
  int offset, count = 0;

  frame = alloc_frame();

  CU_ASSERT(parse_frame(frame, raw_frame, (int)sizeof(raw_frame), &offset) == RET_SUCCESS);

  validate_frame(frame, 2, 24);
}

static void check_parse2() {
  frame_t *frame;
  char raw_frame1[] = "SEND\n"
    "content-length:24\n"
    "some-additional-header:hoge\n";
  char raw_frame2[] = "hoge:fuga\n"
    "\n"
    "(body) hoge\n"
    "(body) fuga\n";
  int offset;

  frame = alloc_frame();

  CU_ASSERT(parse_frame(frame, raw_frame1, (int)sizeof(raw_frame1), &offset) == RET_ERROR);
  CU_ASSERT(parse_frame(frame, raw_frame2, (int)sizeof(raw_frame2), &offset) == RET_SUCCESS);

  validate_frame(frame, 3, 24);
}

static void check_parse3() {
  frame_t *frame;
  char raw_frame1[] = "SEND\n"
    "content-length:24\n"
    "some-additional-header:hoge\n"
    "\n"
    "(body) hoge\n";
  char raw_frame2[] = "(body) fuga\n";
  int offset;

  frame = alloc_frame();

  CU_ASSERT(parse_frame(frame, raw_frame1, (int)sizeof(raw_frame1), &offset) == RET_ERROR);
  CU_ASSERT(parse_frame(frame, raw_frame2, (int)sizeof(raw_frame2), &offset) == RET_SUCCESS);

  validate_frame(frame, 2, 24);
}

int test_frame(CU_pSuite suite) {
  suite = CU_add_suite("FrameTest", NULL, NULL);
  if(suite == NULL) {
    return CU_ERROR;
  }

  CU_add_test(suite, "check_alloc", check_alloc);
  CU_add_test(suite, "check_parse", check_parse);
  CU_add_test(suite, "check_parse2", check_parse2);
  CU_add_test(suite, "check_parse3", check_parse3);

  return CU_SUCCESS;
}

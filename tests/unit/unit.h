#ifndef __UNIT_H__
#define __UNIT_H__

#include <CUnit/CUnit.h>
#include <newt/optparse.h>
#include <newt/stomp.h>
#include <newt/list.h>

#include <test.h>

#include <errno.h>
#include <stdio.h>

int test_optparse(CU_pSuite);
int test_config(CU_pSuite);

extern frame_bucket_t stomp_frame_bucket;

#endif

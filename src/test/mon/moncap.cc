// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2012 Inktank
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include <iostream>

#include "include/stringify.h"
#include "mon/MonCap.h"

#include "gtest/gtest.h"

const char *parse_good[] = {
  "allow *",
  "allow r",
  "allow rwx",
  "allow r pool foo ",
  "allow r pool=foo",
  "allow wx pool taco",
  "allow pool foo r",
  "allow pool taco wx",
  "allow wx pool pool_with_underscores_and_no_quotes",
  "allow rwx pool 'weird name'",
  "allow rwx pool \"weird name with ''s\"",
  "allow rwx pool foo, allow r pool bar",
  "allow rwx pool foo ; allow r pool bar",
  "allow rwx pool foo ;allow r pool bar",
  "allow rwx pool foo; allow r pool bar",
  "allow pool foo rwx, allow pool bar r",
  "allow pool foo rwx ; allow pool bar r",
  "allow pool foo rwx ;allow pool bar r",
  "allow pool foo rwx; allow pool bar r",
  "allow pool data rw, allow pool rbd rwx, allow pool images-foo x",
  "allow pool bar rwx; allow pool baz rx",
  "allow service foo x",
  "allow command \"clsthingidon'tunderstand\" x",
  "  allow rwx pool foo; allow r pool bar  ",
  "  allow   rwx   pool foo; allow r pool bar  ",
  "  allow pool foo rwx; allow pool bar r  ",
  "  allow     pool foo rwx; allow pool bar r  ",
  " allow wx pool taco",
  "allow r   pool    foo   ;   allow   w   service    5 ; allow     command  'asdfasdfasdf asdf'  x   ",
  "allow x pool rbd_children, allow service libvirt-pool-test rwx",
  "allow r pool rbd-children, allow service libvirt_pool_test rwx",
  "allow command 'a b c' x",
  "allow command abc x",
  0
};

TEST(MonCap, ParseGood) {
  for (int i=0; parse_good[i]; i++) {
    string str = parse_good[i];
    MonCap cap;
    std::cout << "Testing good input: '" << str << "'" << std::endl;
    ASSERT_TRUE(cap.parse(str, &cout));
  }
}

const char *parse_bad[] = {
  "allow r poolfoo",
  "allow r w",
  "ALLOW r",
  "allow rwx,",
  "allow rwx x",
  "allow r pool foo r",
  "allow wwx pool taco",
  "allow wwx pool taco^funny&chars",
  "allow rwx pool 'weird name''",
  "allow rwx object_prefix \"beforepool\" pool weird",
  "allow rwx auid 123 pool asdf",
  "allow xrwx pool foo,, allow r pool bar",
  ";allow rwx pool foo rwx ; allow r pool bar",
  "allow rwx pool foo ;allow r pool bar gibberish",
  "allow command asdf",
  0
};

TEST(MonCap, ParseBad) {
  for (int i=0; parse_bad[i]; i++) {
    string str = parse_bad[i];
    MonCap cap;
    std::cout << "Testing bad input: '" << str << "'" << std::endl;
    ASSERT_FALSE(cap.parse(str, &cout));
  }
}

#if 0

TEST(MonCap, AllowAll) {
  MonCap cap;
  ASSERT_FALSE(cap.allow_all());

  ASSERT_TRUE(cap.parse("allow r", NULL));
  ASSERT_FALSE(cap.allow_all());
  cap.grants.clear();

  ASSERT_TRUE(cap.parse("allow w", NULL));
  ASSERT_FALSE(cap.allow_all());
  cap.grants.clear();

  ASSERT_TRUE(cap.parse("allow x", NULL));
  ASSERT_FALSE(cap.allow_all());
  cap.grants.clear();

  ASSERT_TRUE(cap.parse("allow rwx", NULL));
  ASSERT_FALSE(cap.allow_all());
  cap.grants.clear();

  ASSERT_TRUE(cap.parse("allow rw", NULL));
  ASSERT_FALSE(cap.allow_all());
  cap.grants.clear();

  ASSERT_TRUE(cap.parse("allow rx", NULL));
  ASSERT_FALSE(cap.allow_all());
  cap.grants.clear();

  ASSERT_TRUE(cap.parse("allow wx", NULL));
  ASSERT_FALSE(cap.allow_all());
  cap.grants.clear();

  ASSERT_TRUE(cap.parse("allow *", NULL));
  ASSERT_TRUE(cap.allow_all());
  ASSERT_TRUE(cap.is_capable("foo", 0, "asdf", true, true, true, true));
}

TEST(MonCap, AllowPool) {
  MonCap cap;
  bool r = cap.parse("allow rwx pool foo", NULL);
  ASSERT_TRUE(r);

  ASSERT_TRUE(cap.is_capable("foo", 0, "", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "", true, true, true, true));
}

TEST(MonCap, AllowPools) {
  MonCap cap;
  bool r = cap.parse("allow rwx pool foo, allow r pool bar", NULL);
  ASSERT_TRUE(r);

  ASSERT_TRUE(cap.is_capable("foo", 0, "", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "", true, false, false, false));
}

TEST(MonCap, AllowPools2) {
  MonCap cap;
  bool r = cap.parse("allow r, allow rwx pool foo", NULL);
  ASSERT_TRUE(r);

  ASSERT_TRUE(cap.is_capable("foo", 0, "", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "", true, false, false, false));
}

TEST(MonCap, ObjectPrefix) {
  MonCap cap;
  bool r = cap.parse("allow rwx object_prefix foo", NULL);
  ASSERT_TRUE(r);

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "food", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo_bar", true, true, true, true));

  ASSERT_FALSE(cap.is_capable("bar", 0, "_foo", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, " foo ", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "fo", true, true, true, true));
}

TEST(MonCap, ObjectPoolAndPrefix) {
  MonCap cap;
  bool r = cap.parse("allow rwx pool bar object_prefix foo", NULL);
  ASSERT_TRUE(r);

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "food", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo_bar", true, true, true, true));

  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "food", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "fo", true, true, true, true));
}

TEST(MonCap, BasicR) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow r", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, false));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, true, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, false, false));
}

TEST(MonCap, BasicW) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow w", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, true, false, false));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, true, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, false, false));
}

TEST(MonCap, BasicX) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow x", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, true));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, false, false));
}

TEST(MonCap, BasicRW) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow rw", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, false, false));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, true, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
}

TEST(MonCap, BasicRX) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow rx", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, true, true));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, false, false));
}

TEST(MonCap, BasicWX) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow wx", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, true, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, true, true, true));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, false, false));
}

TEST(MonCap, BasicRWX) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow rwx", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, true, false));
}

TEST(MonCap, BasicRWClassRClassW) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow rw class-read class-write", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, true, true, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, true, true, false));
}

TEST(MonCap, ClassR) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow class-read", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, false));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, false, false, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
}

TEST(MonCap, ClassW) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow class-write", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, false, true));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, false, true, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
}

TEST(MonCap, ClassRW) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow class-read class-write", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, false, true));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, true));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
}

TEST(MonCap, BasicRClassR) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow r class-read", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, false));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, false, false));
}

TEST(MonCap, PoolClassR) {
  MonCap cap;
  ASSERT_TRUE(cap.parse("allow pool bar r class-read, allow pool foo rwx", NULL));

  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", false, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, true, false));
  ASSERT_TRUE(cap.is_capable("bar", 0, "foo", true, false, false, false));

  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", false, true, false, false));
  ASSERT_FALSE(cap.is_capable("bar", 0, "foo", true, true, false, false));

  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", false, false, false, false));
  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", false, false, true, true));
  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", true, true, true, true));
  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", false, true, true, true));
  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", true, false, false, true));
  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", true, false, false, false));
  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", true, true, false, true));
  ASSERT_TRUE(cap.is_capable("foo", 0, "foo", true, true, true, false));

  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", false, false, false, false));
  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", false, false, true, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", true, true, true, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", false, true, true, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", true, false, false, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", true, false, false, false));
  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", true, true, false, true));
  ASSERT_FALSE(cap.is_capable("baz", 0, "foo", true, true, true, false));
}

TEST(MonCap, OutputParsed)
{
  struct CapsTest {
    const char *input;
    const char *output;
  };
  CapsTest test_values[] = {
    {"allow *",
     "moncap[grant(*)]"},
    {"allow r",
     "moncap[grant(r)]"},
    {"allow rx",
     "moncap[grant(rx)]"},
    {"allow rwx",
     "moncap[grant(rwx)]"},
    {"allow rw class-read class-write",
     "moncap[grant(rwx)]"},
    {"allow rw class-read",
     "moncap[grant(rw class-read)]"},
    {"allow rw class-write",
     "moncap[grant(rw class-write)]"},
    {"allow rwx pool images",
     "moncap[grant(pool images rwx)]"},
    {"allow r pool images",
     "moncap[grant(pool images r)]"},
    {"allow pool images rwx",
     "moncap[grant(pool images rwx)]"},
    {"allow pool images r",
     "moncap[grant(pool images r)]"},
    {"allow pool images w",
     "moncap[grant(pool images w)]"},
    {"allow pool images x",
     "moncap[grant(pool images x)]"},
    {"allow pool images r; allow pool rbd rwx",
     "moncap[grant(pool images r),grant(pool rbd rwx)]"},
    {"allow pool images r, allow pool rbd rwx",
     "moncap[grant(pool images r),grant(pool rbd rwx)]"},
    {"allow class-read object_prefix rbd_children, allow pool libvirt-pool-test rwx",
     "moncap[grant(object_prefix rbd_children  class-read),grant(pool libvirt-pool-test rwx)]"}
  };

  size_t num_tests = sizeof(test_values) / sizeof(*test_values);
  for (size_t i = 0; i < num_tests; ++i) {
    MonCap cap;
    std::cout << "Testing input '" << test_values[i].input << "'" << std::endl;
    ASSERT_TRUE(cap.parse(test_values[i].input));
    ASSERT_EQ(test_values[i].output, stringify(cap));
  }
}
#endif

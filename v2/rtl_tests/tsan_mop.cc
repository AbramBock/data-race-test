//===-- tsan_mop.cc ---------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//
#include "tsan_interface.h"
#include "tsan_test_util.h"
#include "tsan_report.h"
#include "gtest/gtest.h"
#include <stddef.h>
#include <stdint.h>

using namespace __tsan;  // NOLINT

TEST(ThreadSanitizer, SimpleWrite) {
  ScopedThread t;
  MemLoc l;
  t.Write1(l);
}

TEST(ThreadSanitizer, SimpleWriteWrite) {
  ScopedThread t1, t2;
  MemLoc l1, l2;
  t1.Write1(l1);
  t2.Write1(l2);
}

TEST(ThreadSanitizer, WriteWriteRace) {
  ScopedThread t1, t2;
  MemLoc l;
  t1.Write1(l);
  t2.Write1(l, true);
}

TEST(ThreadSanitizer, ReadWriteRace) {
  ScopedThread t1, t2;
  MemLoc l;
  t1.Read1(l);
  t2.Write1(l, true);
}

TEST(ThreadSanitizer, WriteReadRace) {
  ScopedThread t1, t2;
  MemLoc l;
  t1.Write1(l);
  t2.Read1(l, true);
}

TEST(ThreadSanitizer, ReadReadNoRace) {
  ScopedThread t1, t2;
  MemLoc l;
  t1.Read1(l);
  t2.Read1(l);
}

TEST(ThreadSanitizer, RaceWithOffset) {
  ScopedThread t1, t2;
  {
    MemLoc l;
    t1.Access(l.loc(), true, 8, false);
    t2.Access((char*)l.loc() + 4, true, 4, true);
  }
  {
    MemLoc l;
    t1.Access(l.loc(), true, 8, false);
    t2.Access((char*)l.loc() + 7, true, 1, true);
  }
  {
    MemLoc l;
    t1.Access((char*)l.loc() + 4, true, 4, false);
    t2.Access((char*)l.loc() + 4, true, 2, true);
  }
  {
    MemLoc l;
    t1.Access((char*)l.loc() + 4, true, 4, false);
    t2.Access((char*)l.loc() + 6, true, 2, true);
  }
  {
    MemLoc l;
    t1.Access((char*)l.loc() + 3, true, 2, false);
    t2.Access((char*)l.loc() + 4, true, 1, true);
  }
  {
    MemLoc l;
    t1.Access((char*)l.loc() + 1, true, 8, false);
    t2.Access((char*)l.loc() + 3, true, 1, true);
  }
}

TEST(ThreadSanitizer, RaceWithOffset2) {
  ScopedThread t1, t2;
  {
    MemLoc l;
    t1.Access((char*)l.loc(), true, 4, false);
    const ReportDesc *rep = t2.Access((char*)l.loc() + 2, true, 1, true);
    EXPECT_EQ(rep->mop[0].addr, (uintptr_t)l.loc() + 2);
    EXPECT_EQ(rep->mop[0].size, 1);
    EXPECT_EQ(rep->mop[1].addr, (uintptr_t)l.loc());
    EXPECT_EQ(rep->mop[1].size, 4);
  }
  {
    MemLoc l;
    t1.Access((char*)l.loc() + 2, true, 1, false);
    const ReportDesc *rep = t2.Access((char*)l.loc(), true, 4, true);
    EXPECT_EQ(rep->mop[0].addr, (uintptr_t)l.loc());
    EXPECT_EQ(rep->mop[0].size, 4);
    EXPECT_EQ(rep->mop[1].addr, (uintptr_t)l.loc() + 2);
    EXPECT_EQ(rep->mop[1].size, 1);
  }
}

TEST(ThreadSanitizer, NoRaceWithOffset) {
  ScopedThread t1, t2;
  {
    MemLoc l;
    t1.Access(l.loc(), true, 4, false);
    t2.Access((char*)l.loc() + 4, true, 4, false);
  }
  {
    MemLoc l;
    t1.Access((char*)l.loc() + 3, true, 2, false);
    t2.Access((char*)l.loc() + 1, true, 2, false);
    t2.Access((char*)l.loc() + 5, true, 2, false);
  }
}

TEST(ThreadSanitizer, RaceWithDeadThread) {
  MemLoc l;
  ScopedThread t;
  ScopedThread().Write1(l);
  t.Write1(l, true);
}

TEST(ThreadSanitizer, BenignRaceOnVptr) {
  void *vptr_storage;
  MemLoc vptr(&vptr_storage), val;
  vptr_storage = val.loc();
  ScopedThread t1, t2;
  t1.VptrUpdate(vptr, val);
  t2.Read8(vptr);
}

TEST(ThreadSanitizer, HarmfulRaceOnVptr) {
  void *vptr_storage;
  MemLoc vptr(&vptr_storage), val1, val2;
  vptr_storage = val1.loc();
  ScopedThread t1, t2;
  t1.VptrUpdate(vptr, val2);
  t2.Read8(vptr, true);
}
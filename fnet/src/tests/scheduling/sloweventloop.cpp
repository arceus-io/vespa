// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/fnet/fnet.h>

class MyTask : public FNET_Task
{
public:
  bool        _done;

  MyTask(FNET_Scheduler &scheduler)
    : FNET_Task(&scheduler),
      _done(false) {}

  bool done() const { return _done; }
  void PerformTask() override { _done = true; }
};


TEST("slow event loop") {
  FastOS_Time t;
  t.SetMilliSecs(0);

  FNET_Scheduler scheduler(&t, &t);
  MyTask         task(scheduler);
  MyTask         task2(scheduler);

  scheduler.CheckTasks();
  t.AddMilliSecs(10000);
  task.Schedule(5.0);

  uint32_t cnt = 0;
  for (;;) {
      scheduler.CheckTasks();
      if (task.done()) {
          break;
      }
      ++cnt;
      t.AddMilliSecs(1);
  }

  if (!EXPECT_TRUE(cnt > 4700 && cnt < 4800)) {
      fprintf(stderr, "cnt=%d\n", cnt);
  }

  scheduler.CheckTasks();
  t.AddMilliSecs(10000);
  task2.Schedule(5.0);

  uint32_t cnt2 = 0;
  for(;;) {
      scheduler.CheckTasks();
      if (task2.done()) {
          break;
      }
      ++cnt2;
      t.AddMilliSecs(10000);
  }

  if (!EXPECT_TRUE(cnt2 > 15 && cnt2 < 25)) {
      fprintf(stderr, "cnt2=%d\n", cnt2);
  }
}

TEST_MAIN() { TEST_RUN_ALL(); }

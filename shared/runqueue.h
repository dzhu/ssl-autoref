#pragma once

//==== RunningQueue Class Definition =================================//

#define RUNQUEUE_TEM template <class data_t, const int MaxSize>
#define RUNQUEUE_FUN RunningQueue<data_t, MaxSize>

RUNQUEUE_TEM
class RunningQueue
{
  data_t *arr;
  int start, num;

public:
  RunningQueue()
  {
    arr = 0;
    start = num = 0;
  }
  ~RunningQueue()
  {
    reset();
  }

  bool init();
  void clear()
  {
    start = num = 0;
  }
  void reset()
  {
    delete[](arr);
    arr = 0;
    start = num = 0;
  }

  // add new data to the queue
  void add(const data_t &d);

  // overwriting the last item on the queue with new data
  void overwrite(const data_t &d);

  // get i'th element ago in history, bounded to [-num+1,0]
  const data_t &get(int i) const;
  const data_t &operator()(int i) const
  {
    return (get(i));
  }
  data_t &operator[](int i);

  // truncate the i'th history element and everything after it
  void truncate(int i);

  bool empty() const
  {
    return (!num);
  }
  int size() const
  {
    return (num);
  }
  int getOldestIdx() const
  {
    return (-num + 1);
  }
  bool isValidIdx(int i) const
  {
    return (i > -num && i <= 0);
  }
};

//====================================================================//
//  RunningQueue Implementation
//====================================================================//

RUNQUEUE_TEM
bool RUNQUEUE_FUN::init()
{
  if (arr == 0) arr = new data_t[MaxSize];
  return (arr == 0);
}

RUNQUEUE_TEM
void RUNQUEUE_FUN::add(const data_t &d)
{
  start++;
  if (start >= MaxSize) start = 0;
  if (num < MaxSize) num++;
  arr[start] = d;
}

RUNQUEUE_TEM
void RUNQUEUE_FUN::overwrite(const data_t &d)
{
  if (MaxSize <= 0) return;
  if (num <= 0) num = 1;
  arr[start] = d;
}

RUNQUEUE_TEM
const data_t &RUNQUEUE_FUN::get(int i) const
{
  if (i > 0) i = 0;
  if (i < -num + 1) i = -num + 1;
  return (arr[(start + i + MaxSize) % MaxSize]);
}

RUNQUEUE_TEM
data_t &RUNQUEUE_FUN::operator[](int i)
{
  if (i > 0) i = 0;
  if (i < -num + 1) i = -num + 1;
  return (arr[(start + i + MaxSize) % MaxSize]);
}

RUNQUEUE_TEM
void RUNQUEUE_FUN::truncate(int i)
{
  if (i >= 0) return;
  if (i < -num) i = -num;
  num += i;
}

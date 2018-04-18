#ifndef __TASK_HPP__
#define __TASK_HPP__

#include <deque>

template <class T>
class Task {
public:
  int n;
  std::deque<T> data;

  Task (int n) : n(n) {}
  ~Task () {}

  T & operator()(size_t i) {
    return data[i];
  }
  void push_back(T x) {
    data.push_back(x);
    return;
  }
  T front() const {
    return data.front();
  }
  void pop_front() {
    data.pop_front();
    return;
  }
  size_t size() const {
    return data.size();
  }

  int completed () const {
    return data.size() == n;
  }
  int empty () const {
    return data.size() == 0;
  }
};



#endif

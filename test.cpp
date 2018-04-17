#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <deque>

enum header {TASK_DONE, GIVE_ME, NO_MORE};

class Task {
private:
  int n;
  std::deque<int> data;

public:
  Task (int n) : n(n) {}
  virtual ~Task () {}

  int & operator()(size_t i) {
    return data[i];
  }
  void push_back(int x) {
    data.push_back(x);
    return;
  }
  int front() const {
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

int main(int argc, char *argv[]) {
  int proc_id, world_size;
  // MPI_Status status;
  int incomingMessage;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);



  std::cout << proc_id << "/" << world_size << std::endl;

  if (proc_id == 0) {
    int activeWorkers = world_size-1;
    Task pendingTask(10), completedTask(10);
    for(size_t i=0; i<10; ++i) pendingTask.push_back((int) i);



    while(!completedTask.completed() || activeWorkers) {
      for(int worker=1; worker < world_size; ++worker) {
        MPI_Iprobe(worker, 0, MPI_COMM_WORLD, &incomingMessage,
                MPI_STATUS_IGNORE);
        if(incomingMessage) {
          std::cout << "incoming Message from worker " << worker << std::endl;
          int buffer[2];
          MPI_Recv(buffer, 2, MPI_INT, worker,
                0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          switch (buffer[0]) {
            case TASK_DONE:
              std::cout << "worker " << worker << " has DONE A TASK" << std::endl;
              completedTask.push_back(buffer[1]);
              std::cout << "completed = " << completedTask.size() << std::endl;
              std::cout << "criterio = " << (!completedTask.completed()) << std::endl;
              break;
            case GIVE_ME:
              std::cout << "worker " << worker << " asks for TASK" << std::endl;
              if(!pendingTask.empty()){
                std::cout << "so send task to woker " << worker << std::endl;
                buffer[0] = TASK_DONE;
                buffer[1] = pendingTask.front();
                pendingTask.pop_front();
              } else {
                std::cout << "but no more tasks left for worker " << worker << std::endl;
                buffer[0] = NO_MORE;
                --activeWorkers;
                std::cout << "still active = " << activeWorkers << std::endl;
              }
              MPI_Send(buffer, 2, MPI_INT, worker,
                      0, MPI_COMM_WORLD);
          }
        }
      }
    }
  } else {
    while(1) {
      int buffer[2];
      buffer[0] = GIVE_ME;
      MPI_Send(buffer, 2, MPI_INT, 0,
              0, MPI_COMM_WORLD);
      MPI_Recv(buffer, 2, MPI_INT, 0,
              0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if(buffer[0] == NO_MORE) {
        std::cout << "worker " << proc_id << " finished job" << std::endl;
        break;
      }


      // DO THINGS //
      std::cout << "\tworker " << proc_id << " waits " << buffer[1] << " secs"<< std::endl;
      std::cout << std::flush;
      usleep(buffer[1]*500000);
      //

      buffer[0] = TASK_DONE;
      buffer[1] = 1;
      MPI_Send(buffer, 2, MPI_INT, 0,
              0, MPI_COMM_WORLD);

    }
  }

  std::cout << "proc " << proc_id << " exits" << std::endl;


  MPI_Finalize();
  return 0;
}

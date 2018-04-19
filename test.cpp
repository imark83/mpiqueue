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

  char processor_name[100];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);



  std::cout << proc_id << "/" << world_size << std::endl;

  if (proc_id == 0) {
    int taskSize = 100;
    int activeWorkers = world_size-1;
    Task pendingTask(taskSize), completedTask(taskSize);
    for(size_t i=0; i<taskSize; ++i) pendingTask.push_back((int) 2);



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
              std::cout << "\t\tworker " << worker << " has DONE A TASK" << std::endl;
              completedTask.push_back(buffer[1]);
              std::cout << "\t\tcompleted = " << completedTask.size() << std::endl;
              std::cout << "\t\tcriterio = " << (!completedTask.completed()) << std::endl;
              break;
            case GIVE_ME:
              std::cout << "\t\tworker " << worker << " asks for TASK" << std::endl;
              if(!pendingTask.empty()){
                std::cout << "\t\tso send task to woker " << worker << std::endl;
                buffer[0] = TASK_DONE;
                buffer[1] = pendingTask.front();
                pendingTask.pop_front();
              } else {
                std::cout << "\t\tbut no more tasks left for worker " << worker << std::endl;
                buffer[0] = NO_MORE;
                --activeWorkers;
                std::cout << "\t\t\t\tstill active = " << activeWorkers << std::endl;
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
        std::cout << "\t\t\t\t\t\tworker " << processor_name << ", " << proc_id << " finished job" << std::endl;
        break;
      }


      // DO THINGS //
      std::cout << "\t\t\t\t\t\tworker " << processor_name << "-"<< proc_id << " does " << " task " << buffer[1] << std::endl;
      std::cout << std::flush;
      buffer[1] = 1;
      for(int i=0; i<10000; ++i) for(int j=0; j<10000; ++j)
        buffer[1] += 3*i - j;
      //

      buffer[0] = TASK_DONE;
      //buffer[1] = 1;
      MPI_Send(buffer, 2, MPI_INT, 0,
              0, MPI_COMM_WORLD);

    }
  }

  std::cout << "\t\t\t\tproc " << proc_id << " of " << processor_name << " exits" << std::endl;


  MPI_Finalize();
  return 0;
}

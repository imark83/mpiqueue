#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <cmath>
#include "task.hpp"
enum header {TASK_DONE, GIVE_ME, NO_MORE};

#include <complex>
typedef std::complex<double> Complex;
class Job {
public:
  int i, j;
  Complex z;
  int rop;
  Job () {}
  Job (int i, int j, const Complex &z, int rop) : i(i), j(j), z(z), rop(rop) {}
  Job (int i, int j) : i(i), j(j), z(0), rop(0) {}
  Job (const Job &op) : i(op.i), j(op.j), z(op.z), rop(op.rop) {}
  ~Job() {}

  Job & operator=(const Job &op) {
    i=op.i; j=op.j; z=op.z; rop=op.rop;
    return *this;
  }
};

typedef struct {
  int header;
  Job job;
} buffer_t;

int doTask (Complex z) {
  Complex root[3];
  root[0] = Complex(1,0);
  root[1] = Complex(-0.5, sqrt(3.0)/2);
  root[2] = Complex(-0.5,-sqrt(3.0)/2);

  for(int iter=0; iter<1000; ++iter) {
    z = z - (z*z*z-1.0)/(3.0*z*z);
    for(int k=0; k<3; ++k)
      if(std::abs(z-root[k]) < 0.1)
        return k+1;
  }
}


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
  buffer_t buffer;
  size_t bufferSize = sizeof(buffer);


  std::cout << proc_id << "/" << world_size << std::endl;
  int M = 16*1024;
  int N = 16*1024;
  Complex z0(-3.0, -3.0);
  Complex z1(3.0, 3.0);

  if (proc_id == 0) {
    int activeWorkers = world_size-1;



    // DECLARE TASKS TO ASSIGN AND COMPLETED TASKS
    Task<Job> pendingTask(M*N);
    Task<Job> completedTask(M*N);
    for(int i=0; i<N; ++i) for(int j=0; j<M; ++j) {
      Job job(i, j);
      Complex delta(j*std::real(z1-z0)/(M-1.0), i*std::imag(z1-z0)/(N-1.0));
      job.z = z0 + delta;
      pendingTask.push_back(job);
    }



    while(!completedTask.completed() || activeWorkers) {
      for(int worker=1; worker < world_size; ++worker) {
        MPI_Iprobe(worker, 0, MPI_COMM_WORLD, &incomingMessage,
                MPI_STATUS_IGNORE);
        if(incomingMessage) {
          std::cout << "incoming Message from worker " << worker << std::endl;
          MPI_Recv(&buffer, sizeof(buffer_t), MPI_BYTE, worker,
                0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          switch (buffer.header) {
            case TASK_DONE:
              std::cout << "worker " << worker << " has DONE A TASK" << std::endl;
              completedTask.push_back(buffer.job);
              break;
            case GIVE_ME:
              std::cout << "worker " << worker << " asks for TASK" << std::endl;
              if(!pendingTask.empty()){
                buffer.header = TASK_DONE;
                buffer.job = pendingTask.front();
                pendingTask.pop_front();
              } else {
                std::cout << "\tbut there are no more tasks left for worker "
                        << worker << std::endl;
                buffer.header = NO_MORE;
                --activeWorkers;
              }
              MPI_Send(&buffer, sizeof(buffer_t), MPI_BYTE, worker,
                      0, MPI_COMM_WORLD);
          }
        }
      }
    }
    // while loop complete.

    // STORE TASKS
    int *rop = new int[M*N];
    std::deque<Job>::iterator it;
    for(it = completedTask.data.begin(); it != completedTask.data.end(); *it++)
      rop[M*(it->i) + (it->j)] = it->rop;

    FILE *fout = fopen("newton.bin", "wb");
    fwrite(rop, sizeof(int), M*N, fout);
    fclose(fout);
    delete [] rop;

  }

  // WORKERS
  if (proc_id != 0) {
    while(1) {
      buffer.header = GIVE_ME;
      MPI_Send(&buffer, sizeof(buffer_t), MPI_BYTE, 0,
              0, MPI_COMM_WORLD);
      MPI_Recv(&buffer, sizeof(buffer_t), MPI_BYTE, 0,
              0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if(buffer.header == NO_MORE) {
        std::cout << "worker " << processor_name << ", " << proc_id << " finished job" << std::endl;
        break;
      }


      // DO THINGS //
      buffer.job.rop = doTask(buffer.job.z);
      //

      buffer.header = TASK_DONE;
      MPI_Send(&buffer, sizeof(buffer_t), MPI_BYTE, 0,
              0, MPI_COMM_WORLD);

    }
  }

  std::cout << "proc " << proc_id << " of " << processor_name << " exits" << std::endl;




  MPI_Finalize();
  return 0;
}

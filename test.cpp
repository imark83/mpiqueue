#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <deque>
#include <complex>
#include <string.h>

typedef std::complex<double> Complex;
enum header {TASK_DONE, GIVE_ME, NO_MORE, GIVE_YOU};


inline int min(int a, int b) {
  if(a<b) return a;
  return b;
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


  Complex z0(-3,-3), z1(3,3);
  const size_t max_chunkSize = 3;
  int taskSize = 7;

  // COMMUNICATION BUFFER. MAX SIZE = max_chunkSize + 3
  // header
  // index
  // chunkSize
  // data
  int *buffer = new int[max_chunkSize+3];

  // STATUS STRUCT FOR COMMUNICATION
  MPI_Status status;
  // message length
  int msgLen;


  // ROP
  int *rop;

  std::cout << proc_id << "/" << world_size << std::endl;

  if (proc_id == 0) {
    int activeWorkers = world_size-1;
    int pendingTask = taskSize;
    int completedTask = 0;
    rop = new int[taskSize];

    // Task pendingTask(taskSize), completedTask(taskSize);


    while(!completedTask || activeWorkers) {
      for(int worker=1; worker < world_size; ++worker) {
        MPI_Iprobe(worker, 0, MPI_COMM_WORLD, &incomingMessage,
                &status);
        if(incomingMessage) {
          // get length
          MPI_Get_count(&status, MPI_INT, &msgLen);
          std::cout << "incoming Message from worker " << worker << std::endl;
          MPI_Recv(buffer, msgLen, MPI_INT, worker,
                0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          switch (buffer[0]) {
            case TASK_DONE:
              std::cout << "\t\tworker " << worker << " has DONE A TASK of len " << msgLen << std::endl;

              // UPLOAD DATA
              memcpy(rop+buffer[1], buffer+3, sizeof(int) * buffer[2]);
              completedTask += buffer[2];
              // END UPLOAD

              std::cout << "\t\tcompleted = " << completedTask << std::endl;
              std::cout << "\t\tcriterio = " << (completedTask != taskSize)
                      << std::endl;
              break;
            case GIVE_ME:
              std::cout << "\t\tworker " << worker << " asks for TASK" << std::endl;
              if(pendingTask){
                std::cout << "\t\tso send task to woker " << worker << std::endl;
                buffer[0] = GIVE_YOU;
                buffer[1] = taskSize-pendingTask;
                buffer[2] = min(max_chunkSize, pendingTask);
                pendingTask -= buffer[2];
              } else {
                std::cout << "\t\tbut no more tasks left for worker " << worker << std::endl;
                buffer[0] = NO_MORE;
                --activeWorkers;
                std::cout << "\t\t\t\tstill active = " << activeWorkers << std::endl;
              }
              MPI_Send(buffer, 3, MPI_INT, worker,
                      0, MPI_COMM_WORLD);
          }
        }
      }
    }
  } else {
    while(1) {
      buffer[0] = GIVE_ME;
      MPI_Send(buffer, 1, MPI_INT, 0,
              0, MPI_COMM_WORLD);
      MPI_Recv(buffer, 3, MPI_INT, 0,
              0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if(buffer[0] == NO_MORE) {
        std::cout << "\t\t\t\t\t\tworker " << processor_name << ", " << proc_id << " finished job" << std::endl;
        break;
      }


      // DO THINGS //
      std::cout << "\t\t\t\t\tworker " << processor_name << "-"<< proc_id << " does " << " task " << buffer[1] << " of len " << buffer[2] << std::endl;
      std::cout << std::flush;

      for(int i=0; i<buffer[2]; ++i)
        buffer[3+i] = (buffer[1]+i) * (buffer[1]+i);

      buffer[0] = TASK_DONE;
      //buffer[1] = 1;
      MPI_Send(buffer, buffer[2]+3, MPI_INT, 0,
              0, MPI_COMM_WORLD);

    }
  }

  if(proc_id == 0) {
    FILE *fout = fopen("newton.bin", "wb");
    fwrite(rop, sizeof(int), taskSize, fout);
    fclose(fout);
  }


  MPI_Finalize();
  return 0;
}

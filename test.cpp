#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <cstdlib>
#include <string.h>
#include <vector>

using namespace std;


enum _header {TASK_DONE, GIVE_ME, NO_MORE, GIVE_YOU};


const double par0[] = {2.0, 3.0};
const double par1[] = {-3.0, -2.0};
int M = 4;
int N = 3;



inline int min(int a, int b) {
  if(a<b) return a;
  return b;
}


typedef struct _Task {
  int header;           // COMMUNICATION PROTOCOL
  int index;
  int i;
  int j;
  double par0;           // INPUT
  double par1;           // INPUT

  int latidos;
  int picos;
  double xf[27];
} Task;




ostream & operator<<(ostream &output, const struct _Task &op) {
  output << op.index << " " << op.i << " " << op.j << " (" << op.latidos << ", " << op.picos << ") " << op.xf[0] << " " << op.xf[26] << " <---> par = " << op.par0 << ", " << op.par1;

  return output;
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



  // COMMUNICATION BUFFER.
  Task buffer;
  vector<Task> rop(0);

  // STATUS STRUCT FOR COMMUNICATION
  MPI_Status status;


  cout << proc_id << "/" << world_size << endl;

  if (proc_id == 0) {
    int activeWorkers = world_size-1;
    int taskSize = M*N;
    int pendingTask = M*N;
    int completedTask = 0;
    rop.resize(taskSize);

    // INITIALIZE TASK INPUT
    for(int i=0; i<M; ++i) for(int j=0; j<N; ++j) {
      rop[j*M+i].i = i;
      rop[j*M+i].j = j;
      rop[j*M+i].index = j*M+i;
      rop[j*M+i].par0 = par0[0] + i*((par0[1]-par0[0])/(M-1.0));
      rop[j*M+i].par1 = par1[0] + j*((par1[1]-par1[0])/(N-1.0));

      rop[j*M+i].latidos = -1;
      rop[j*M+i].picos = -1;
    }

    // Task pendingTask(taskSize), completedTask(taskSize);


    while(activeWorkers) {
      for(int worker=1; worker < world_size; ++worker) {
        MPI_Iprobe(worker, 0, MPI_COMM_WORLD, &incomingMessage,
                &status);
        if(incomingMessage) {
          // get length
          cout << "incoming Message from worker " << worker << endl;
          MPI_Recv(&buffer, sizeof(Task), MPI_CHAR, worker,
                0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          switch (buffer.header) {
            case TASK_DONE:
              cout << "\t\tworker " << worker << " has DONE A TASK " << buffer.index << endl;
              // UPLOAD DATA
              rop[buffer.index] = buffer;
              completedTask += 1;
              // END UPLOAD
              cout << "\t\tcompleted = " << completedTask << endl;
              cout << "\t\taskSize = " << taskSize << endl;
              cout << "\t\tcriterio = " << (completedTask != taskSize)
                      << endl;
              break;
            case GIVE_ME:
              cout << "\t\tworker " << worker << " asks for TASK" << endl;
              if(pendingTask>0){
                cout << "\t\tso send task to woker " << worker << endl;
                for(int i=0; i<M; ++i) {
                  int index = taskSize-pendingTask;
                  buffer = rop[index];
                  buffer.header = GIVE_YOU;
                  --pendingTask;
                  MPI_Send(&buffer, sizeof(Task), MPI_CHAR, worker,
                    0, MPI_COMM_WORLD);
                }

              } else {
                cout << "\t\tbut no more tasks left for worker " << worker << endl;
                buffer.header = NO_MORE;
                --activeWorkers;
                cout << "\t\t\t\tstill active = " << activeWorkers << endl;
                MPI_Send(&buffer, sizeof(Task), MPI_CHAR, worker,
                  0, MPI_COMM_WORLD);
              }
          }
        }
      }
    }
  } else {
    while(1) {
      vector<Task> lineBuffer(M);

      lineBuffer[0].header = GIVE_ME;
      MPI_Send(&lineBuffer[0], sizeof(Task), MPI_CHAR, 0,
              0, MPI_COMM_WORLD);
      MPI_Recv(&lineBuffer[0], sizeof(Task), MPI_CHAR, 0,
              0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if(lineBuffer[0].header == NO_MORE) {
        cout << "\t\t\t\t\t\tworker " << processor_name << ", " << proc_id << " finished job" << endl;
        break;
      }
      // HAY MAS TAREAS
      for(int i=1; i<M; ++i) {
        MPI_Recv(&lineBuffer[i], sizeof(Task), MPI_CHAR, 0,
        0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        lineBuffer[i] = buffer;
      }


      cout << "buffer 0 pars = " << lineBuffer[0].par0 << ", " << lineBuffer[0].par1 << endl;

      // DO THINGS //
      // LLAMAR AL INTEGRADOR

      // for(int j=0; j<27; ++j)
      //   lineBuffer[0].xf[j] = 10.0;
      lineBuffer[0].latidos = 2;
      lineBuffer[0].picos = 30000 + buffer.index;
      lineBuffer[0].header = TASK_DONE;
      //
      // std::cout << "\t\t pp = " << lineBuffer[0].par0 << std::endl;
      //
      for(int i=1; i<M; ++i) {
      //   for(int j=0; j<27; ++j)
      //     lineBuffer[i].xf[j] = lineBuffer[i-1].xf[j] + 10.0;
        lineBuffer[i].latidos = 2;
        lineBuffer[i].picos = 30000 + buffer.index;
        lineBuffer[i].header = TASK_DONE;
      }

      cout << "buffer 0 pars = " << lineBuffer[0].par0 << ", " << lineBuffer[0].par1 << endl;

      for(int i=0; i<M; ++i)
        MPI_Send(&lineBuffer[i], sizeof(Task), MPI_CHAR, 0,
              0, MPI_COMM_WORLD);

    }
  }

  usleep(1000);
  if(proc_id == 0) {
    for(int i=0; i<M*N; ++i)
      cout << rop[i] << endl;
  }

  MPI_Finalize();
  return 0;
}

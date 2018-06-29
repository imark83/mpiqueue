#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <cstdlib>
#include <string.h>
#include <vector>

using namespace std;


enum _header {TASK_DONE, GIVE_ME, NO_MORE, GIVE_YOU};


const double par[] = {2.0, 3.0};
int N = 1001;
int nvar=27;



inline int min(int a, int b) {
  if(a<b) return a;
  return b;
}


typedef struct _Task {
  int header;           // COMMUNICATION PROTOCOL
  int index;
  double par;           // INPUT
  double x0[27];        // INPUT

  double xf[27];        // OUTPUT
  int latidos;
  int picos;
} Task;




ostream & operator<<(ostream &output, const struct _Task &op) {
  output << "(" << op.latidos << ", " << op.picos << ") " << op.xf[0] << " " << op.xf[nvar-1] << " <---> par = " << op.par;

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
    int taskSize = N;
    int pendingTask = N;
    int completedTask = 0;
    rop.resize(N);

    // INITIALIZE TASK INPUT
    for(int i=0; i<N; ++i) {
      rop[i].index = i;
      rop[i].par = par[0] + i*((par[1]-par[0])/(N-1.0));
      for(int j=0; j<nvar; ++j)
        rop[i].x0[j] = rop[i].xf[j] = -1.0;
      rop[i].latidos = -1;
      rop[i].picos = -1;
    }

    // Task pendingTask(taskSize), completedTask(taskSize);


    while(!completedTask || activeWorkers) {
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
              cout << "\t\tworker " << worker << " has DONE A TASK" << endl;

              // UPLOAD DATA
              rop[buffer.index] = buffer;
              // END UPLOAD
              cout << "\t\tcompleted = " << completedTask << endl;
              cout << "\t\tcriterio = " << (completedTask != taskSize)
                      << endl;
              ++completedTask;
              break;
            case GIVE_ME:
              cout << "\t\tworker " << worker << " asks for TASK" << endl;
              if(pendingTask){
                cout << "\t\tso send task to woker " << worker << endl;
                int index = taskSize-pendingTask;
                buffer = rop[index];
                buffer.header = GIVE_YOU;
                pendingTask -= 1;

              } else {
                cout << "\t\tbut no more tasks left for worker " << worker << endl;
                buffer.header = NO_MORE;
                --activeWorkers;
                cout << "\t\t\t\tstill active = " << activeWorkers << endl;
              }
              MPI_Send(&buffer, sizeof(Task), MPI_CHAR, worker,
                      0, MPI_COMM_WORLD);
          }
        }
      }
    }
  } else {
    while(1) {
      buffer.header = GIVE_ME;
      MPI_Send(&buffer, sizeof(Task), MPI_CHAR, 0,
              0, MPI_COMM_WORLD);
      MPI_Recv(&buffer, sizeof(Task), MPI_CHAR, 0,
              0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if(buffer.header == NO_MORE) {
        cout << "\t\t\t\t\t\tworker " << processor_name << ", " << proc_id << " finished job" << endl;
        break;
      }

      // DO THINGS //
      // LLAMAR AL INTEGRADOR
      for(int j=0; j<nvar; ++j)
        buffer.xf[j] = buffer.par;
      buffer.latidos = 2;
      buffer.picos = 30000 + buffer.index;


      buffer.header = TASK_DONE;
      //buffer[1] = 1;
      MPI_Send(&buffer, sizeof(Task), MPI_CHAR, 0,
              0, MPI_COMM_WORLD);

    }
  }

  usleep(1000);
  if(proc_id == 0) {
    for(int i=0; i<N; ++i)
      cout << rop[i] << endl;
  }

  MPI_Finalize();
  return 0;
}

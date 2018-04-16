#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
  int proc_id, mpi_size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);



  printf("%i / %i\n", proc_id, mpi_size);

  if (proc_id == 0) {
    MPI_Request request[mpi_size];    
    int i = 0;
    int task[10] = {1,2,3,4,5,6,7,8,9,10};
    while (i<10) {

    }
  } else {
    MPI_Request request;
    do {

    } while ();
  }

  MPI_Status status[mpi_size];


  MPI_Finalize();
  return 0;
}

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>

#include "mat.h"

int main(int argc, char* argv[])
{
    int myid, numprocs;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if (argc > 1) {

        if (myid == 0) {// Controller Code goes here
            
        } else { // Worker code goes here
            
        }
    } else {
        fprintf(stderr, "Usage invalid <size>\n");
    }

    MPI_Finalize();
    return 0;
}
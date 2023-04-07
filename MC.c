#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>


#include "mat.h"

int main(int argc, char* argv[])
{
    int nrows, ncols;
    double *aa;    /* the A matrix */
    double *bb;    /* the B matrix */
    double *cc1;    /* A x B computed using the omp-mpi code you write */
    double *cc2;    /* A x B computed using the conventional algorithm */
    int myid, numprocs;
    double starttime, endtime;
    MPI_Status status;

    /* insert other global variables here */
    int stripesize;
    int numsent = 0;
    double *a, *buffer;

    FILE * fp;
    fp = fopen ("data.txt", "a");

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if (argc > 1) {
        //get and set number of rows and columns
        nrows = atoi(argv[1]);
        ncols = nrows;
        

        //set stripesize to number of slaves
        stripesize = ncols/3;

        //malloc for buffer, a, and b
        buffer = (double*)malloc(ncols * stripesize * sizeof(double));
        a = (double*)malloc(sizeof(double) * nrows * stripesize);
        aa = (double*)malloc(sizeof(double) * nrows * ncols);
        bb = (double*)malloc(sizeof(double) * nrows * ncols);

        if (myid == 0) {// Controller Code goes here
            fprintf(fp, "%d ", ncols);
            //generate matrices to multiply together
            aa = gen_matrix(nrows, ncols);
            bb = gen_matrix(ncols, nrows);

            //malloc for cc1(answer matrix)
            cc1 = malloc(sizeof(double) * nrows * ncols);

            //initialize for loop iterators
            int i, j, k, l;

            //start mpi timing
            starttime = MPI_Wtime();
            /* Insert your controller code here to store the product into cc1 */

            //broadcast bb (the matrix that each stripe is getting multiplied by)
            MPI_Bcast(bb, nrows * ncols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            //for loop to send each stripe to a slave
            for(k = 0; k < 3; k++) {
                for (i = 0; i < stripesize; i++) {
                    for (j = 0; j < ncols; j++) {
                        buffer[i*ncols + j] = aa[(i + k * stripesize) * ncols + j];
                    }
                }
                MPI_Send(buffer, ncols * stripesize, MPI_DOUBLE, k+1, k, MPI_COMM_WORLD);
                numsent++;
            }

            //receive stripes
            for (i = 0; i < 3; i++) {
                MPI_Recv(buffer, ncols * stripesize, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, 
                MPI_COMM_WORLD, &status);
            
                //get the stripe number
                int stripe = status.MPI_TAG;

                //insert the stripe into the answer matrix cc1
                for (j = 0; j < stripesize; j++) {
                    for (k = 0; k < nrows; k++) {
                        cc1[(j + stripe * stripesize) * nrows + k] = buffer[j * nrows + k];
                    }
                }
            }
            
            //end MPI timing
            endtime = MPI_Wtime();
            printf("Calculating MPI OMP size: %d, time = %f\n", ncols, (endtime - starttime));
            fprintf(fp, "%f\n",(endtime - starttime));

            //compare matrices with normal mmult
            // cc2  = malloc(sizeof(double) * nrows * nrows);
            // mmult(cc2, aa, nrows, ncols, bb, ncols, nrows);
            // compare_matrices(cc2, cc1, nrows, nrows);
        } else { // Worker code goes here
            
            //malloc buffer, a, and  for slaves
            buffer = (double*)malloc(ncols * stripesize * sizeof(double));
            a = (double*)malloc(sizeof(double) * nrows * stripesize);

            //broadcast matrix bb (the matrix that each stripe is getting multiplied by)
            MPI_Bcast(bb, nrows * ncols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            //recieve buffer
            MPI_Recv(buffer, ncols * stripesize, MPI_DOUBLE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            int stripe = status.MPI_TAG;

            //omp matrix mult of buffer(stripe) and bb to a
            int i, j, k = 0;
            #pragma omp parallel default(none) shared(a, bb, buffer, stripesize, ncols, nrows) private(i, k, j,stripe)
            #pragma omp for
            for (i = 0; i < stripesize; i++) {
                for (j = 0; j < nrows; j++) {
                    a[i * nrows + j] = 0;
                }
                for (k = 0; k < ncols; k++) {
                    for (j = 0; j < nrows; j++) {
                        a[i * nrows + j] += buffer[i * ncols + k] * bb[k * nrows + j];
                    }
                }
            }
            
            //send stripe back to controller
            MPI_Send(a, nrows * stripesize, MPI_DOUBLE, 0, stripe, MPI_COMM_WORLD);
        }
    } else {
        fprintf(stderr, "Usage mmult_mpi_omp <size>\n");
    }
    MPI_Finalize();

    fclose(fp);
    return 0;
}
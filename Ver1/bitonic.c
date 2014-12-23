#include "utilities.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_all_arrays(int *array, int N, int processID, int numTasks)
{
    int runs = 0;
    while (runs < numTasks) {
        if (processID == runs) {
            printf ("Array printed by rank: %d\n", processID);
            print_array(array, N);
            fflush (stdout);
        }
        runs ++;
        MPI_Barrier (MPI_COMM_WORLD);
    }
}

int main(int argc , char** argv)
{
    // ID of the process.
    int processID;

    int *array;
    // The number of elements in the local data array.
    int N;
    int numTasks;
    int i, j ;
    int p , q ;

    // Check if enough arguments were passed to each process.
    if ( argc < 3) {
        // If not print a warning message with the correct way to use
        // the program and terminate the execution.
        printf("Invalid command line argument option! \n");
        printf("Usage : %s p q where p is the number of MPI processes to "
               "be spawned and q the number of elements in each process. \n ",
               argv[0] );
        exit(ARG_ERROR);
    }

    // Initialize the MPI processes and pass the command line
    // arguments to every one of them.
    MPI_Init( &argc , &argv);

    // Get the number of tasks running
    MPI_Comm_size( MPI_COMM_WORLD , &numTasks);

    // Get the id of the process from the MPI daemon.
    MPI_Comm_rank( MPI_COMM_WORLD , &processID);

    // Parse the command line arguments.
    p = atoi( argv[1]);
    q = atoi( argv[2]);

    // The master process checks that the correct number of processes
    // has started working.
    if (processID == 0 && numTasks != ( 1 << p) ) {
        printf("The number of tasks is not equal to the one passed to "
               "the master process and thus the sorting procedure will stop! \n");

        // Terminate the MPI processes.
        MPI_Abort(MPI_COMM_WORLD, NTHREADS_ERROR);
        exit(NTHREADS_ERROR);
    }

    /*Calculate the total number of elements.
     * and create the dataset */
    N = 1 << q ;

    printf("Process %d has started generating the dataset! \n", processID);
    array = (int *)malloc( N * sizeof(int) );

    if ( array == NULL ) {
        printf("Process %d failed to allocate the memory required for the"
               " data array \n", processID);
        MPI_Abort(MPI_COMM_WORLD, MALLOC_ERROR);
        exit(MALLOC_ERROR);
    }

    srand(time(NULL) * (processID + 1));
    for ( i = 0 ; i < N ; i++ ) {
        array[i] = rand() % N ;
    }

    /* Sort the local data in ascending order */

    qsort( array , N , sizeof(int) , ascendingOrder );

    // Wait for all the tasks to generate the data set.
    MPI_Barrier(MPI_COMM_WORLD);

    // Communicate with all corresponding processes.
   
    for (int i = 0 ; i < p ; i++) {
        for (int  j = i ; j >= 0 ; j-- ) {
            /* The next process we will be paired with is the one
             * whose j-th bit is the complement of ours. So we toggle
             * our j-th bit using the XOR operation to find our partner
            */
            int partner = processID ^ (1 << j);
            /* Check if we have to keep the smallest out of our elements
             * and our partner's or the largest.
             * We do this by checking the (i+1) bit of our id and comparing it
             * to the #j bit. If they are equal then we keep the small
             * elements , if not the larger ones.
            */
            if ( (( processID >> (i + 1)) & 1  ) == (  (processID >> j) & 1) )
              compare(&array, N, partner, ASCENDING);
            else 
              compare(&array, N, partner, DESCENDING);

        }      
    } // End of for loop for communications.

    // Wait for all the processes to finish the exchanges.
    MPI_Barrier(MPI_COMM_WORLD);

    // If the test macro is enabled print that the sorting finished
    // successfully and perform verification tests.
    #ifdef TEST

    // Size of the receiving buffer used for testing.
    int final_size = N * numTasks;

    // Pointer to the array of the received data from every process.
    int *final;
    // Only the master process needs to allocate memory.
    if (processID == MASTER)
    {
      final = malloc(final_size * sizeof(int));

      if ( final == NULL )
      {
        printf("Could not allocate memory for the buffer so as to " 
            "receive all the data. The test will not be performed! \n ");
      }
    }
    
    /* If we are the master process gather all the data :
     * array : local array of every process 
     * N : number of elements each process has
     * final : receiving buffer
     * N : Number of elements the receiving process will get from every
     *     other process in the sorting network.
    */

    MPI_Gather(array, N,MPI_INT , final, N, MPI_INT, 0, MPI_COMM_WORLD);

    if (processID == MASTER && final != NULL ) {
    
      // Call the test routines to see if the procedure was successful.
      int pass = ascendingSort( final , final_size ) ;

      free(final);
    }

    #endif
   
    free(array);
    MPI_Finalize();
    return 0;

}


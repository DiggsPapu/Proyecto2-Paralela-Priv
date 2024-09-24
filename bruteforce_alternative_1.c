#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <openssl/des.h>
#include <time.h>

int main(int argc, char *argv[]) {
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

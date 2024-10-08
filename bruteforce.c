#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

void decrypt_message(long key, unsigned char *ciph, int len){
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_DECRYPT);
}

void encrypt_message(long key, unsigned char *ciph, int len){
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

char search[] = "es una prueba de";

int tryKey(long key, unsigned char *ciph, int len){
    unsigned char temp[len+1];
    memcpy(temp, ciph, len);
    temp[len] = 0;

    decrypt_message(key, temp, len);
    return strstr((char *)temp, search) != NULL;
}

unsigned char plaintext[] = "Esta es una prueba de proyecto 2";
unsigned char cipher[32]; // espacio para el texto cifrado

int main(int argc, char *argv[]){ 
    int N, id;
    long upper = (1L << 56); // upper bound DES keys 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int flag;

    int ciphlen = strlen((char *)plaintext);
    memcpy(cipher, plaintext, ciphlen);

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    int range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }

    // Encrypt the message with a test key (for testing purposes)
    long testKey = 1234567890; // You can modify this key as needed
    encrypt_message(testKey, cipher, ciphlen);

    long found = 0;
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        if (tryKey(i, cipher, ciphlen)) {
            found = i;
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    if (id == 0) {
        MPI_Wait(&req, &st);
        decrypt_message(found, cipher, ciphlen);
        printf("Found key: %li\nDecrypted text: %s\n", found, cipher);
    }

    MPI_Finalize();
    return 0;
}

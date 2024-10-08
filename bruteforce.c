#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <time.h>  // Para medir el tiempo

void add_padding(unsigned char *input, int *len) {
    int padding = 8 - (*len % 8);  // El tamaño de bloque DES es 8 bytes
    for (int i = 0; i < padding; ++i) {
        input[*len + i] = padding; // Rellenar con el número de bytes de padding
    }
    *len += padding;
}

void remove_padding(unsigned char *input, int *len) {
    int padding = input[*len - 1]; // Obtener el valor de padding
    *len -= padding;               // Restar el valor de padding
}

void decrypt_message(long key, unsigned char *ciph, int *len){
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    // Desencriptar en bloques de 8 bytes
    for (int i = 0; i < *len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_DECRYPT);
    }
    remove_padding(ciph, len); // Eliminar padding después del descifrado
}

void encrypt_message(long key, unsigned char *ciph, int *len){
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    add_padding(ciph, len); // Agregar padding antes del cifrado

    // Encriptar en bloques de 8 bytes
    for (int i = 0; i < *len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_ENCRYPT);
    }
}

char search[] = "es una prueba de";

int tryKey(long key, unsigned char *ciph, int len){
    unsigned char temp[len+1];
    memcpy(temp, ciph, len);
    temp[len] = 0;

    decrypt_message(key, temp, &len);
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
    long testKey = 1234567890; // Puedes modificar esta clave si lo deseas
    encrypt_message(testKey, cipher, &ciphlen);

    long found = 0;
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    // Comenzar a medir el tiempo de descifrado
    clock_t start, end;
    double decrypt_time;

    start = clock();  // Inicio del tiempo

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

        end = clock();  // Fin del tiempo
        decrypt_time = ((double) (end - start)) / CLOCKS_PER_SEC;

        printf("Decryption took: %f seconds\n", decrypt_time);  // Tiempo total de descifrado
        printf("Decrypted with key: %li\n", found);  // Imprimir la clave encontrada
        decrypt_message(found, cipher, &ciphlen);
        printf("Decrypted text: %s\n", cipher);
    }

    MPI_Finalize();
    return 0;
}

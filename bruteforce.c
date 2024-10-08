//bruteforce.c
//nota: el key usado es bastante pequenio, cuando sea random speedup variara

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h> 

void decrypt(long key, char *ciph, int len) {
    
    DES_key_schedule schedule;
    DES_cblock des_key;
    memset(des_key, 0, 8); 
    memcpy(des_key, &key, sizeof(long)); 
    DES_set_key_unchecked(&des_key, &schedule);  

    for (int i = 0; i < len; i += 8) {  
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_DECRYPT);
    }
}

void encrypt(long key, char *ciph, int len) {
    DES_key_schedule schedule;
    DES_cblock des_key;
    memset(des_key, 0, 8);
    memcpy(des_key, &key, sizeof(long));
    DES_set_key_unchecked(&des_key, &schedule);

    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_ENCRYPT);
    }
}


char search[] = " the ";
int tryKey(long key, char *ciph, int len){
  char temp[len+1];
  memcpy(temp, ciph, len);
  temp[len]=0;
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};
int main(int argc, char *argv[]){ //char **argv

    FILE *file = fopen("textoACifrar.txt", "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *plain_text = malloc(file_size + 1);
    if (!plain_text) {
        perror("Memory allocation error");
        fclose(file);
        return 1;
    }

    fread(plain_text, 1, file_size, file);
    plain_text[file_size] = '\0';
    fclose(file);
    int plain_text_len = strlen(plain_text);
    char cipher[plain_text_len + 1];
    memcpy(cipher, plain_text, plain_text_len + 1);
    
    // Imprimir el mensaje original antes del cifrado
    printf("Texto original: %s\n", plain_text);
    
    encrypt(123456L, cipher, plain_text_len);
    
    // Imprimir el texto cifrado
    printf("Texto cifrado: ");
    for (int i = 0; i < plain_text_len; ++i) {
        printf("%02x ", (unsigned char)cipher[i]);
    }
    printf("\n");
    strcpy(search, "es una prueba de");
    int N, id;
    long upper = (1L <<56); //upper bound DES keys 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int flag;
    int ciphlen = strlen(cipher);
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    int range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id+1) -1;
    if(id == N-1){
        //compensar residuo
        myupper = upper;
    }

    long found = 0;

    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        // Imprimir cada llave que se está probando, por ejemplo, cada 1000000 llaves para reducir el output
        if (i % 1000000 == 0) {
            printf("Proceso %d probando llave: %li\n", id, i);
            fflush(stdout); // Asegurar que se imprima inmediatamente
        }

        if (tryKey(i, (char *)cipher, ciphlen)) {
            found = i;
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }


    if(id==0){
        MPI_Wait(&req, &st);
        decrypt(found, (char *)cipher, ciphlen);
        printf("%li %s\n", found, cipher);
    }

    MPI_Finalize();
}
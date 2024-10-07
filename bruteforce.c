#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <mpi.h>
#include <openssl/provider.h>

void handleErrors(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    ERR_print_errors_fp(stderr);  // Print OpenSSL error messages
    exit(EXIT_FAILURE);
}
int decrypt(long key, char *ciph, int len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create EVP_CIPHER_CTX.\n");
        return 0; // Return 0 to indicate failure
    }

    unsigned char key_block[8];
    for (int i = 0; i < 8; ++i) {
        key_block[i] = (key >> (8 * (7 - i))) & 0xFF;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_des_ecb(), NULL, key_block, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return 0; // Return 0 to indicate failure
    }

    int outlen1, outlen2;
    unsigned char plaintext[len + 8];  // Ensure enough space for padding
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &outlen1, (unsigned char *)ciph, len)) {
        EVP_CIPHER_CTX_free(ctx);
        return 0; // Return 0 to indicate failure
    }

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + outlen1, &outlen2)) {
        EVP_CIPHER_CTX_free(ctx);
        return 0; // Return 0 to indicate failure
    }

    memcpy(ciph, plaintext, outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return 1; // Return 1 to indicate success
}

void encrypt(long key, char *ciph, int len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleErrors("Failed to create EVP_CIPHER_CTX.");

    unsigned char key_block[8];
    for (int i = 0; i < 8; ++i) {
        key_block[i] = (key >> (8 * (7 - i))) & 0xFF;
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_des_ecb(), NULL, key_block, NULL))
        handleErrors("EVP_EncryptInit_ex failed.");

    int outlen1, outlen2;
    unsigned char ciphertext[len + 8];  // Ensure enough space for padding
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &outlen1, (unsigned char *)ciph, len))
        handleErrors("EVP_EncryptUpdate failed.");
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + outlen1, &outlen2))
        handleErrors("EVP_EncryptFinal_ex failed.");

    memcpy(ciph, ciphertext, outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
}


char search[] = " the ";
int tryKey(long key, char *ciph, int len) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    if (decrypt(key, temp, len)) {  // Only check for the substring if decryption is successful
        return strstr((char *)temp, search) != NULL;
    }
    return 0;  // Return 0 if decryption failed
}


unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};
int main(int argc, char *argv[]) {
    if (!OSSL_PROVIDER_load(NULL, "legacy")) {
        fprintf(stderr, "Fallo al cargar el proveedor legacy.\n");
        exit(EXIT_FAILURE);
    }

    int N, id;
    long upper = (1L << 56); // upper bound for DES keys: 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int ciphlen = sizeof(cipher) - 1; // Correct length of cipher array
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    long range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }

    printf("Proceso %d buscando claves en el rango: %ld - %ld\n", id, mylower, myupper);

    long found = 0;
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        if (i % 1000000 == 0) {
            printf("Proceso %d probando clave: %ld\n", id, i);
        }

        if (tryKey(i, (char *)cipher, ciphlen)) {
            found = i;
            printf("Proceso %d encontró la clave correcta: %ld\n", id, i);
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    if (id == 0) {
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        decrypt(found, (char *)cipher, ciphlen);
        printf("Found key: %li Decrypted message: %s\n", found, cipher);
    }

    MPI_Finalize();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <openssl/des.h>
#include <omp.h>
#include <time.h>

void add_padding(unsigned char *input, int *len) {
    int padding = 8 - (*len % 8);
    for (int i = 0; i < padding; ++i) {
        input[*len + i] = padding; // Fill with the number of padding bytes
    }
    *len += padding;
}

void remove_padding(unsigned char *input, int *len) {
    int padding = input[*len - 1];
    *len -= padding;
}

void decrypt_message(long key, unsigned char *ciph, int *len) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    for (int i = 0; i < *len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_DECRYPT);
    }
    remove_padding(ciph, len);
}

void encrypt_message(long key, unsigned char *ciph, int *len) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    add_padding(ciph, len);

    for (int i = 0; i < *len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_ENCRYPT);
    }
}

int tryKey(long key, unsigned char *ciph, int len, const char *search) {
    unsigned char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0; // Ensure the string is null-terminated

    decrypt_message(key, temp, &len);
    temp[len] = '\0'; // Explicitly null-terminate the decrypted string

    if (strstr((char *)temp, search) != NULL) {
        return 1; // Key found
    }
    return 0; // Key not found
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file> <key> <search_string>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    long key = atol(argv[2]);
    char *search = argv[3];

    int N, id, flag;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    // Read the content of the file
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Could not open the file");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *plaintext = malloc(filesize);
    fread(plaintext, 1, filesize, file);
    fclose(file);

    unsigned char *cipher = malloc(filesize + 8); // +8 for padding
    int ciphlen = filesize;
    memcpy(cipher, plaintext, ciphlen);

    // Encrypt the message with the provided key
    encrypt_message(key, cipher, &ciphlen);

    long upper = (1L << 56);
    long mylower = (upper / N) * id;
    long myupper = (upper / N) * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }
    printf("Process %d is responsible for key range: [%li - %li]\n", id, mylower, myupper);
    long found = -1; // Initialize as -1 to indicate that no key has been found
    int key_found = 0; // Flag to indicate if the key is found
    double start_time = MPI_Wtime(); // Start timing

    // Loop to search for the key
    for (long i = mylower; i <= myupper; ++i) {
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE);
        if (flag) { // Check if a key has been found
            MPI_Recv(&key_found, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE);
            if (key_found) {
                break; // Exit if a key has already been found
            }
        }

        if (tryKey(i, cipher, ciphlen, search)) {
            found = i; // Key found
            key_found = 1; // Set the key_found flag
            for (int j = 0; j < N; ++j) {
                MPI_Send(&key_found, 1, MPI_INT, j, 0, comm); // Inform all processes
            }
            break;
        }
    }

    if (found != -1) {
        double end_time = MPI_Wtime(); // End timing
        printf("Key found: %li by process %d\n", found, id);
        decrypt_message(found, cipher, &ciphlen);
        cipher[ciphlen] = '\0'; // Ensure null-terminated string
        printf("Decrypted text: %s\n", cipher);
        printf("Decryption time: %f seconds\n", end_time - start_time);
    }

    free(plaintext);
    free(cipher);
    MPI_Finalize();
    return EXIT_SUCCESS;
}

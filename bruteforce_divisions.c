#include <openssl/des.h>
#include <mpi.h>

int main(int argc, char *argv[]) {

    int divisions = atoi(argv[4]);
    long range_per_division = (myupper - mylower + 1) / divisions;
    long ranges[divisions][2];

    for (int i = 0; i < divisions; i++) {
        ranges[i][0] = mylower + i * range_per_division;
        ranges[i][1] = (i == divisions - 1) ? myupper : ranges[i][0] + range_per_division - 1;
    }

    int N, id;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

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

    encrypt_message(key, cipher, &ciphlen);

    long upper = (1L << 56);
    long mylower = (upper / N) * id;
    long myupper = (upper / N) * (id + 1) - 1;

    if (id == N - 1) {
        myupper = upper;
    }

    printf("Process %d is responsible for key range: [%li - %li]\n", id, mylower, myupper);

    for (long i = 0; ; i++) {
        for (int j = 0; j < divisions; j++) {
            long candidate;
            if (i % 2 == 0) {
                candidate = ranges[j][0] + i / 2;
            } else {
                candidate = ranges[j][1] - i / 2;
            }
            if (candidate < ranges[j][0] || candidate > ranges[j][1]) continue;
            if (tryKey(candidate, cipher, ciphlen, search)) {
                found = candidate;
                key_found = 1;
                break;
            }
        }
        if (key_found) break;
    }

    int key_found = 0;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE);
    if (flag) {
        MPI_Recv(&key_found, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE);
        if (key_found) break;
    }
    if (found != -1) {
        for (int j = 0; j < N; ++j) {
            MPI_Send(&key_found, 1, MPI_INT, j, 0, comm);
        }
    }

    if (found != -1) {
        double end_time = MPI_Wtime();
        printf("Key found: %li by process %d\n", found, id);
        decrypt_message(found, cipher, &ciphlen);
        cipher[ciphlen] = '\0';
        printf("Decrypted text: %s\n", cipher);
        printf("Decryption time: %f seconds\n", end_time - start_time);
    }


    MPI_Finalize();
    return 0;
}


void add_padding(unsigned char *input, int *len) {
    int padding = 8 - (*len % 8);
    for (int i = 0; i < padding; ++i) {
        input[*len + i] = padding;
    }
    *len += padding;
}

void remove_padding(unsigned char *input, int *len) {
    int padding = input[*len - 1];
    *len -= padding;
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

int tryKey(long key, unsigned char *ciph, int len, const char *search) {
    unsigned char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;

    decrypt_message(key, temp, &len);
    temp[len] = '\0';

    if (strstr((char *)temp, search) != NULL) {
        return 1;
    }
    return 0;
}

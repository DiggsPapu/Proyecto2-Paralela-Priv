#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/des.h> 
#include <time.h>  // Para medir el tiempo de ejecución

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

int tryKey(long key, char *ciph, int len) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr(temp, search) != NULL;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};

int main() {
    long upper = (1L << 56);  // (2^56)
    int ciphlen = strlen((const char *)cipher);
    long found = 0;

    // Medición del tiempo de inicio
    clock_t start_time = clock();

    for (long i = 0; i < upper; ++i) {
        if (tryKey(i, (char *)cipher, ciphlen)) {
            found = i;
            break;
        }
    }

    // Medición del tiempo final
    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    if (found) {
        decrypt(found, (char *)cipher, ciphlen);
        printf("Key found: %li, Decrypted text: %s\n", found, cipher);
    } else {
        printf("Key not found.\n");
    }

    // Imprimir el tiempo transcurrido
    printf("Time taken: %.2f seconds\n", time_spent);

    return 0;
}

#include <string.h>
#include <stdio.h>
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

int tryKey(long key, char *ciph, int len) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr(temp, search) != NULL;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};

int main() {
    int ciphlen = strlen((const char *)cipher);
    for (long i = 0; i < (1L << 56); ++i) {
        if (tryKey(i, (char *)cipher, ciphlen)) {
            printf("Key found: %li\n", i);
            break;
        }
    }
    return 0;
}


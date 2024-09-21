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

int main() {
    return 0;
}

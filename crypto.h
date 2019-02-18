void init_crypto(char* NetworkPassword);
int encrypt_packet(unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext);
int decrypt_packet(unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext);

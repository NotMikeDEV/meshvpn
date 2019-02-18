#include "globals.h"
#include "crypto.h"

unsigned char *iv = (unsigned char *)"0000000000000000";

int encrypt_packet(unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext)
{
	EVP_CIPHER_CTX *ctx;

	int len = 0;

	int ciphertext_len = 0;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new()))
		ERR_print_errors_fp(stderr);

	/* Initialise the encryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, network_key, iv))
	      ERR_print_errors_fp(stderr);

	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(!EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		ERR_print_errors_fp(stderr);
	ciphertext_len = len;

	/* Finalise the encryption. Further ciphertext bytes may be written at
	 * this stage.
	 */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
		ERR_print_errors_fp(stderr);

	ciphertext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

int decrypt_packet(unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext)
{
  EVP_CIPHER_CTX *ctx;

  int len = 0;

  int plaintext_len = 0;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new()))
  	  perror("EVP_CIPHER_CTX_new");

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, network_key, iv))
	  perror("EVP_DecryptInit_ex");


  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
	  perror("EVP_DecryptUpdate");
  plaintext_len = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
	printf("Error decrypting packet\n");

  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}

void init_crypto(char* NetworkPassword)
{
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	SHA256(NetworkPassword, strlen(NetworkPassword), network_key);

	if (Debug)
	{
		int i;
		printf("Network Key: ");
		for (i = 0; i < sizeof(network_key); i++) {
		    printf("%02x", network_key[i]);
		}
		printf("\n");
	}
}

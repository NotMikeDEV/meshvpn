class Crypto
{
private:
	static unsigned char NetworkKey[32];
public:
	static void Init(unsigned char* NetworkKey);
	static int Decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext);
	static int Encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext);
};
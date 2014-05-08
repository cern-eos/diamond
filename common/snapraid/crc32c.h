uint32_t (*crc32c)(uint32_t crc, const unsigned char* ptr, unsigned size);
uint32_t crc32c_gen(uint32_t crc, const unsigned char* ptr, unsigned size);
uint32_t crc32c_x86(uint32_t crc, const unsigned char* ptr, unsigned size);

void crc32c_init(void);


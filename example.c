#include "gooseberry.h"

// FNV 5.0.2
#define FNV1_32_INIT ((uint32_t)0x811c9dc5)
#define FNV_32_PRIME ((uint32_t)0x01000193)

uint32_t fnv_32_buf(uint8_t *buffer, uintptr len, uint32_t hval)
{
    return (buffer[0] == 0) ? FNV1_32_INIT : fnv_32_buf(&buffer[1], ( FNV1_32_INIT * FNV_32_PRIME ) ^ (uint32_t)buffer[0], FNV1_32_INIT);
}

// This example only supports 32-bit hashes. Don't use this in production!
void gooseberry_hash_wrapper(uint8_t* name, uintptr name_size, uint8_t* hash_out, uintptr hash_size)
{
    *(uint32_t*)hash_out = fnv_32_buf(name, name_size, FNV1_32_INIT);
}

int main()
{
    uint8_t bytes[1000];

    // Carefully set the size and limit to avoid any bugs.
    struct gooseberry_context context = { 0 };
    context.buffer = &bytes;
    context.size = 1000;
    context.max_tag_count = 70; 


    struct gooseberry_tag_info simple_encoding = { 0 };

    uint32_t u32 = 1234;
    simple_encoding.tag.bits.type = GOOSEBERRY_TYPE_UINT;
    simple_encoding.tag.bits.size = GOOSEBERRY_SIZE_32;
    simple_encoding.tag.bits.has_extension = 0;
    simple_encoding.data = &u32;

    if(!gooseberry_try_write_tag(&context, &simple_encoding))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }

    if(context.index != 5)
    {
        // the simple_encoding should be 5 bytes, so the index should be 5.
    }

    context.hash_name = &gooseberry_hash_wrapper;
    
    struct gooseberry_tag_info harder_encoding = { 0 };
    harder_encoding.tag.bits.type = GOOSEBERRY_TYPE_UINT;
    harder_encoding.tag.bits.size = GOOSEBERRY_SIZE_64;

    uint64_t u64 = 4321;
    harder_encoding.tag.bits.has_extension = 1;
    harder_encoding.extension.bits.has_known_endianess = 0;
    harder_encoding.extension.bits.has_name = 1;
    harder_encoding.extension.bits.name_hash_size = GOOSEBERRY_SIZE_32;

    unsigned char* name = "test";
    uint32_t test_hash = fnv_32_buf((uint8_t*)name, 4, FNV1_32_INIT);

    harder_encoding.hash = &test_hash;
    harder_encoding.data = &u64;

    harder_encoding.extension.bits.is_nested = 0;

    if(!gooseberry_try_write_tag(&context, &harder_encoding))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }

    if(context.index != 19)
    {
        // This harder encoding is 14 bytes. The total index should be 19.
    }

    struct gooseberry_tag_info complex_encoding = { 0 };
    complex_encoding.tag.bits.type = GOOSEBERRY_TYPE_BIN;
    complex_encoding.tag.bits.size = GOOSEBERRY_SIZE_8;

    uint8_t bin[5] = { 1, 2, 3, 4, 5};

    harder_encoding.tag.bits.has_extension = 1;
    harder_encoding.extension.bits.has_known_endianess = 0;
    harder_encoding.extension.bits.has_name = 1;
    harder_encoding.extension.bits.name_hash_size = GOOSEBERRY_SIZE_32;

    harder_encoding.bin_size_tag_size = 1;
    harder_encoding.bin_size_tag[0] = 5;
    harder_encoding.bin_data = &bin;

    if(!gooseberry_try_write_tag(&context, &harder_encoding))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }

    if(context.index != 33)
    {
        // This harder encoding is 14 bytes. The total index should be 33.
    }

    // Now let's try to decode data.
    context.index = 0;

    struct gooseberry_tag_info decode_test =  { 0 };

    if(!gooseberry_try_read_next_tag(&context, &decode_test))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }

    if((decode_test.tag.byte != simple_encoding.tag.byte)
    || (*(uint32_t*)decode_test.data != 1234u)
    || (decode_test.total_size != 5)
    || (context.index != 5))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }

    if(!gooseberry_try_read_next_tag(&context, &decode_test))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }

    if((decode_test.tag.byte != harder_encoding.tag.byte)
    || (decode_test.extension.byte != harder_encoding.extension.byte)
    || (!gooseberry_memcmp(&test_hash, decode_test.hash, 4))
    || (*(uint64_t*)decode_test.data != 4321ull)
    || (decode_test.total_size != 14)
    || (context.index != 19))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }

    if((decode_test.tag.byte != harder_encoding.tag.byte)
    || (decode_test.extension.byte != harder_encoding.extension.byte)
    || (!gooseberry_memcmp(&test_hash, decode_test.hash, 4))
    || (!gooseberry_memcmp(&bin, decode_test.bin_data, 5))
    || (!decode_test.bin_size_tag_size != 1) // The size tag size should be 1 byte.
    || (decode_test.total_size != 14)
    || (context.index != 33))
    {
        // When there's an error, "reset" the context (and tag_info sometimes with other functions).
        // Zero everything except the buffer, size, and max_tag_count.
    }
}
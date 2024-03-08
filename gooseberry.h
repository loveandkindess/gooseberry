#include <stdint.h>

#define GOOSEBERRY_LIBRARY_VERSION 2
#define GOOSEBERRY_FORMAT_VERSION 2

#define GOOSEBERRY_HEADER "Gooseberry"
#define GOOSEBERRY_ENDIANNESS_TEST 0x01020304 // This is intended to be a uint32_t

#if __SIZEOF_POINTER__ == 2
typedef uint16_t uintptr;
#elif __SIZEOF_POINTER__ == 4
typedef uint32_t uintptr;
#elif __SIZEOF_POINTER__ == 8
typedef uint64_t uintptr;
#endif

uint8_t gooseberry_memcmp(void* buffer1, void* buffer2, uintptr size);

void gooseberry_memset(void* buffer, uint8_t byte, uintptr size);

void gooseberry_memcpy(void* from, void* to, uintptr size);


enum GOOSEBERRY_FORMAT_TYPE
{
    GOOSEBERRY_ONE_OR_TWO_BYTES = 0,
    GOOSEBERRY_TWO_OR_THREE_BYTES = 1
};

enum GOOSEBERRY_TYPE
{
    GOOSEBERRY_TYPE_UINT = 0,
    GOOSEBERRY_TYPE_SINT = 1,
    GOOSEBERRY_TYPE_FLOAT = 2,
    GOOSEBERRY_TYPE_BFLOAT = 3,
    GOOSEBERRY_TYPE_POSIT = 4,
    GOOSEBERRY_TYPE_QUIRE = 5,
    GOOSEBERRY_TYPE_ARRAY = 6, // Indicates there is supposed to be XYZ types ahead.
    GOOSEBERRY_TYPE_BIN = 7 // You can use this as a string.
};

enum GOOSEBERRY_SIZE
{
    GOOSEBERRY_SIZE_8 = 0,
    GOOSEBERRY_SIZE_16 = 1,
    GOOSEBERRY_SIZE_32 = 2,
    GOOSEBERRY_SIZE_64 = 3,
    GOOSEBERRY_SIZE_128 = 4,
    GOOSEBERRY_SIZE_256 = 5,
    GOOSEBERRY_SIZE_512 = 6,
    GOOSEBERRY_SIZE_1024 = 7,

    GOOSEBERRY_SIZE_24 = 8,
    GOOSEBERRY_SIZE_48 = 9,
    GOOSEBERRY_SIZE_80 = 10,
    GOOSEBERRY_SIZE_112 = 11,
    GOOSEBERRY_SIZE_224 = 12,
    GOOSEBERRY_SIZE_384 = 13,
    GOOSEBERRY_SIZE_768 = 14,
    GOOSEBERRY_SIZE_2048 = 15
};

enum GOOSEBERRY_ENDIANESS
{
    GOOSEBERRY_LITTLE_ENDIAN = 0,
    GOOSEBERRY_BIG_ENDIAN = 1,
    GOOSEBERRY_HONEYWELL_ENDIAN = 2,
    GOOSEBERRY_PDP_ENDIAN = 3
};

uintptr GOOSEBERRY_TYPE_SIZES[16] = { 1, 2, 4, 8,
                                   16, 32, 64, 128,
                                   3, 4, 10, 14,
                                   28, 48, 96, 256 };

union gooseberry_tag
{
    uint8_t byte;
    struct
    {
        uint8_t type : 3;
        uint8_t size : 4;
        uint8_t has_extension : 1;
    } bits;
};

union gooseberry_tag_extension
{
    uint8_t byte;
    struct
    {
        uint8_t has_name : 1;
        uint8_t name_hash_size : 4;
        uint8_t is_nested : 1; // Does this contain another Gooseberry tag?
        uint8_t has_known_endianess : 1;
        uint8_t is_little_endian : 1; // If not, the data is most likely big endian.
    } bits;
};

union gooseberry_2_or_3_byte_tag
{
    uint16_t byte;
    struct
    {
        uint8_t type : 3;
        uint8_t size : 4;
        uint8_t extra_class_byte : 1; // This byte can be anything, it's up to you..

        uint8_t has_name : 1;
        uint8_t name_hash_size : 4;
        uint8_t is_nested : 1; // Does this contain another Gooseberry tag?
        uint8_t endianess : 2;
    } bits;
};

struct gooseberry_tag_info
{
    uint8_t format;

    uint8_t* buffer;

    union gooseberry_tag tag;
    union gooseberry_tag_extension extension;

    union gooseberry_2_or_3_byte_tag big_tag;
    uint8_t extra_class_byte;

    uint8_t* hash;

    uintptr bin_size_tag_size;
    uint8_t bin_size_tag[sizeof(uintptr)];
    uint8_t* bin_data; // This is the data after the size tag.
    
    uint8_t* data;

    uintptr total_size;
};

struct gooseberry_context
{
    uint8_t format;

    uint8_t* buffer;
    uintptr index;
    uintptr size;

    uintptr hash_size;
    void (*hash_name)(uint8_t* name, uintptr name_size, uint8_t* hash_out, uintptr hash_size);

    /* This is a temporary buffer used to hash names. It is used to save memory and time.
     Set this to the size of the name hashes you expect, or dynamically allocate this.
     Don't forget to know it's size. */
    uint8_t temp_name_hash[64];

    uintptr max_tag_count;
    uintptr tag_counts[8];
};

uintptr gooseberry_add_counts(uintptr* counts, uintptr count_count);

uint8_t gooseberry_range_within_bounds(struct gooseberry_context* context, uintptr length);

uint8_t gooseberry_get_bin_info(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info, uint8_t variable_size);

uint8_t gooseberry_peek_next_tag(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info);

uint8_t gooseberry_tag_within_bounds(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info);

uint8_t gooseberry_try_write_tag(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info);

uint8_t gooseberry_try_read_next_tag(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info);

// This will return the first match with the name hash.
uint8_t gooseberry_search_by_name(struct gooseberry_context *context, uint8_t *name, uintptr name_length, struct gooseberry_tag_info *tag_info, uintptr range_start, uintptr range_end, struct gooseberry_tag_info **result);

// Set tag_info to 0 to only get the tag count. *tag_count must be zero.
// If the tag limit is zero, it will count as many tags as possible. Be careful.
uint8_t gooseberry_list_all_tags(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info, uintptr* tag_count, uintptr tag_limit);

// Are the next N tags of a specific type and/or size?
// If they aren't N tags ahead, 0 is returned.
uint8_t gooseberry_are_next_tags(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info, uintptr tag_count, uint8_t match_type, uint8_t match_size);

Gooseberry is a data serialization format like MessagePack, Protobufs, and Avro. Gooseberry supports a wider range of number formats, including bfloat16 and Unums (posits and quires). Various number sizes ranging from 8 to 2048 bits (yes!) are supported.

I have implemented this in C, due to performance and the native support of unions and bit fields.

I ask you all to port this to other programming languages.

Unlike Protobufs and Avro, Gooseberry is very simple and will always be. Let's go straight to the encoding.

Gooseberry has 8 types:
enum GOOSEBERRY_TYPE
{
    GOOSEBERRY_TYPE_UINT = 0,
    GOOSEBERRY_TYPE_SINT = 1,
    GOOSEBERRY_TYPE_FLOAT = 2,
    GOOSEBERRY_TYPE_BFLOAT = 3, (brain floating point)
    GOOSEBERRY_TYPE_POSIT = 4, (Type III Unum)
    GOOSEBERRY_TYPE_QUIRE = 5, (Type III Unum)
    GOOSEBERRY_TYPE_ARRAY = 6, // Indicates there is supposed to be XYZ types ahead
    GOOSEBERRY_TYPE_BIN = 7 // You can use this as a string
};

For UINT, SINT, and FLOAT, the encoding is not specified.

There are 16 type sizes:
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

This is the structure of a Gooseberry tag:
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

If has_extension is set, the next byte is this tag:
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

If has_name is set, the next bytes are a hash of one of the 16 sizes. As of Feb. 5 2024, I recommend BLAKE3-512 (XOF), SHA3-512, or xxHash3 (non-cryptographic).

If the type is GOOSEBERRY_TYPE_BIN, the next bytes are a uint of gooseberry_tag.bits.size bytes indicating the size of the data. This is called the size tag. The next bytes are supposed to be the rest of the data with the indicated size.

If the type isn't the GOOSEBERRY_TYPE_BIN type, the next bytes are the data, as long as gooseberry_tag.bits.size indication.

By the way, the overall endianness of the encoding was never specified. That is up to you.

An emphasis was placed on the arrangement/structure of the data instead of the byte and bit order.
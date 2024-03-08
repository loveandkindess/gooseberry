#include "gooseberry.h"

uint8_t gooseberry_memcmp(void* buffer1, void* buffer2, uintptr size)
{
    for(uintptr i = 0; i < size; i++)
    {
        if(*(uint8_t*)(buffer1 + i) != *(uint8_t*)(buffer2 + i))
        {
            return 0;
        }
    }

    return 1;
}

void gooseberry_memset(void* buffer, uint8_t byte, uintptr size)
{
    for(uintptr i = 0; i < size; i++)
    {
        *(uint8_t*)(buffer + i) = byte;
    }
}

void gooseberry_memcpy(void* from, void* to, uintptr size)
{
    for(uintptr i = 0; i < size; i++)
    {
        *(uint8_t*)(to + i) = *(uint8_t*)(from + i);
    }
}



uintptr gooseberry_add_counts(uintptr* counts, uintptr count_count)
{
    uintptr result = 0;

    for(uintptr i = 0; i < count_count; i++)
    {
        result += counts[i];
    }

    return result;
}

uint8_t gooseberry_range_within_bounds(struct gooseberry_context* context, uintptr length)
{
    if ((context->buffer + context->index + length)
        > (context->buffer + context->size))
    {
        return 0;
    }

    return 1;
}

uint8_t gooseberry_get_bin_part_info(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info, uint8_t variable_size)
{
    if(gooseberry_range_within_bounds(context, GOOSEBERRY_TYPE_SIZES[variable_size]))
    {
        return 0;
    }

    switch(variable_size)
    {
        case GOOSEBERRY_SIZE_8:
        tag_info->bin_size_tag_size = 1;
        *(uint8_t*)tag_info->bin_size_tag = *(uint8_t*)(context->buffer + context->index);
        tag_info->bin_data = (uint8_t*)(context->buffer + context->index + 1);
        
        if(!gooseberry_range_within_bounds(context, tag_info->bin_size_tag_size + tag_info->bin_size_tag))
        {
            return 0;
        }

        return 1;
        break;
#if __SIZEOF_POINTER__ >= 2
        case GOOSEBERRY_SIZE_16:
        tag_info->bin_size_tag_size = 2;
        *(uint8_t*)tag_info->bin_size_tag = *(uint16_t*)(context->buffer + context->index);
        tag_info->bin_data = (uint8_t*)(context->buffer + context->index + 2);
        
        if(!gooseberry_range_within_bounds(context, tag_info->bin_size_tag_size + tag_info->bin_size_tag))
        {
            return 0;
        }

        return 1;
        break;
#endif
#if __SIZEOF_POINTER__ >= 4
        case GOOSEBERRY_SIZE_32:
        tag_info->bin_size_tag_size = 4;
        *(uint8_t*)tag_info->bin_size_tag = *(uint32_t*)(context->buffer + context->index);
        tag_info->bin_data = (uint8_t*)(context->buffer + context->index + 4);
        
        if(!gooseberry_range_within_bounds(context, tag_info->bin_size_tag_size + tag_info->bin_size_tag))
        {
            return 0;
        }

        return 1;
        break;
#endif
#if __SIZEOF_POINTER__ >= 8
        case GOOSEBERRY_SIZE_64:
        tag_info->bin_size_tag_size = 8;
        *(uint8_t*)tag_info->bin_size_tag = *(uint64_t*)(context->buffer + context->index);
        tag_info->bin_data = (uint8_t*)(context->buffer + context->index + 8);
        
        if(!gooseberry_range_within_bounds(context, tag_info->bin_size_tag_size + tag_info->bin_size_tag))
        {
            return 0;
        }

        return 1;
        break;
#endif
        default:
        return 0;
        break;
    }
}

uint8_t gooseberry_peek_next_tag(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info)
{
    if(!gooseberry_range_within_bounds(context, 1))
    {
        return 0;
    }

    tag_info->tag = *(union gooseberry_tag*)(context->buffer + context->index);

    if(tag_info->tag.bits.has_extension)
    {
        if(!gooseberry_range_within_bounds(context, 2))
        {
            return 0;
        }

        tag_info->extension = *(union gooseberry_tag_extension*)(context->buffer + context->index + 1);
    }

    return 1;
}

uint8_t gooseberry_tag_within_bounds(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info)
{
    if(!gooseberry_range_within_bounds(context, 1))
    {
        return 0;
    }

    uintptr total_size = 1;

    uint8_t has_name = 0;

    if(tag_info->tag.bits.has_extension)
    {
        if(!gooseberry_range_within_bounds(context, 2))
        {
            return 0;
        }

        total_size++;

        if(tag_info->extension.bits.has_name)
        {
            has_name = 1;
        }
    }

    if(has_name)
    {
        total_size += GOOSEBERRY_TYPE_SIZES[tag_info->extension.bits.name_hash_size];
    }

    if(tag_info->tag.bits.type == GOOSEBERRY_TYPE_BIN)
    {
        total_size += (tag_info->bin_size_tag_size + *(uintptr*)&tag_info->bin_size_tag);
    }
    
    total_size += GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size];

    if(!gooseberry_range_within_bounds(context, total_size))
    {
        return 0;
    }

    return 1;
}

uint8_t gooseberry_try_write_tag(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info)
{
    if((gooseberry_add_counts(&context->tag_counts, 8) + 1) > context->max_tag_count)
    {
        return 0;
    }

    if(!gooseberry_tag_within_bounds(context, tag_info))
    {
        return 0;
    }

    *(union gooseberry_tag*)(context->buffer + context->index) = tag_info->tag;
    context->index++;

    uint8_t has_name = 0;

    if(tag_info->tag.bits.has_extension)
    {
        if(!gooseberry_range_within_bounds(context, 2))
        {
            return 0;
        }

        *(union gooseberry_tag_extension*)(context->buffer + context->index) = tag_info->extension;
        context->index++;

        if(tag_info->extension.bits.has_name)
        {
            has_name = 1;
        }
    }

    if(has_name)
    {
        gooseberry_memcpy(tag_info->hash, 
        (void*)(context->buffer + context->index)
        , GOOSEBERRY_TYPE_SIZES[tag_info->extension.bits.name_hash_size]);

        context->index += GOOSEBERRY_TYPE_SIZES[tag_info->extension.bits.name_hash_size];
    }

    if(tag_info->tag.bits.type == GOOSEBERRY_TYPE_BIN)
    {
        
        gooseberry_memcpy(&tag_info->bin_size_tag, 
        (void*)(context->buffer + context->index)
        , tag_info->bin_size_tag_size);

        context->index += tag_info->bin_size_tag_size;

        gooseberry_memcpy(&tag_info->bin_data, 
        (void*)(context->buffer + context->index)
        , *(uintptr*)&tag_info->bin_size_tag);

        context->index += *(uintptr*)&tag_info->bin_size_tag;
    }

    gooseberry_memcpy(&tag_info->data, 
        (void*)(context->buffer + context->index)
        , GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size]);

    context->index += GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size];

    context->tag_counts[tag_info->tag.bits.type]++;
    return 1;
}

uint8_t gooseberry_try_read_next_tag(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info)
{
    if(!gooseberry_range_within_bounds(context, 1))
    {
        return 0;
    }

    tag_info->total_size = 1;

    tag_info->tag = *(union gooseberry_tag*)(context->buffer + context->index);
    context->index++;

    uint8_t has_name = 0;

    if(tag_info->tag.bits.has_extension)
    {
        if(!gooseberry_range_within_bounds(context, 2))
        {
            return 0;
        }

        tag_info->extension = *(union gooseberry_tag_extension*)(context->buffer + context->index);
        context->index++;
        tag_info->total_size++;

        if(tag_info->extension.bits.has_name)
        {
            has_name = 1;
        }
    }

    if(has_name)
    {
        if(!gooseberry_range_within_bounds(context, GOOSEBERRY_TYPE_SIZES[tag_info->extension.bits.name_hash_size]))
        {
            return 0;
        }

        tag_info->hash = (uint8_t*)(context->buffer + context->index);
        context->index += GOOSEBERRY_TYPE_SIZES[tag_info->extension.bits.name_hash_size];
        tag_info->total_size += GOOSEBERRY_TYPE_SIZES[tag_info->extension.bits.name_hash_size];
    }

    if(tag_info->tag.bits.type == GOOSEBERRY_TYPE_BIN)
    {
        if(!gooseberry_range_within_bounds(context, GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size]))
        {
            return 0;
        }

        tag_info->bin_size_tag_size = GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size];

        // Can we parse this specific tag size?
        if(GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size] > __SIZEOF_POINTER__)
        {
            return 0;
        }

        gooseberry_memcpy((void*)(context->buffer + context->index),
        &tag_info->bin_size_tag,
        tag_info->bin_size_tag_size);

        context->index += tag_info->bin_size_tag_size;
        tag_info->total_size += *(uintptr*)&tag_info->bin_size_tag_size;

        if(!gooseberry_range_within_bounds(context, *(uintptr*)&tag_info->bin_size_tag))
        {
            return 0;
        }

        tag_info->bin_data = (uint8_t*)(context->buffer + context->index);

        context->index += *(uintptr*)&tag_info->bin_size_tag;
        tag_info->total_size += *(uintptr*)&tag_info->bin_size_tag;
    }

    if(!gooseberry_range_within_bounds(context, GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size]))
    {
        return 0;
    }

    tag_info->data= (uint8_t*)(context->buffer + context->index);

    context->index += GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size];
    tag_info->total_size += GOOSEBERRY_TYPE_SIZES[tag_info->tag.bits.size];

    return 0;
}

// This will return the first match with the name hash.
uint8_t gooseberry_search_by_name(struct gooseberry_context *context, uint8_t *name, uintptr name_length, struct gooseberry_tag_info *tag_info, uintptr range_start, uintptr range_end, struct gooseberry_tag_info **result)
{
    struct gooseberry_tag_info current_tag = { 0 };

    for(uintptr i = range_start; i < range_end; i++)
    {
        if(tag_info[i].tag.bits.has_extension)
        {
            if(context->hash_size == GOOSEBERRY_TYPE_SIZES[tag_info[i].extension.bits.name_hash_size])
            {
                if(tag_info[i].extension.bits.has_name)
                {
                    context->hash_name(name, name_length, context->temp_name_hash, context->hash_size);
        
                    if(gooseberry_memcmp((void*)&tag_info[i].hash, (void*)context->temp_name_hash, context->hash_size))
                    {
                        *result = &tag_info[i];
                        return 1;
                    }
                }
            }
            
        }
    }

    return 0;
}

// Set tag_info to 0 to only get the tag count. *tag_count must be zero.
// If the tag limit is zero, it will count as many tags as possible. Be careful.
uint8_t gooseberry_list_all_tags(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info, uintptr* tag_count, uintptr tag_limit)
{
    struct gooseberry_context context_copy = { 0 };
    gooseberry_memcpy((void*)context, (void*)&context_copy, sizeof(context_copy));

    struct gooseberry_tag_info current_tag = { 0 };

    do
    {
       *tag_count++;

        if(tag_info)
        {
            gooseberry_memcpy((void*)&current_tag, (void*)&tag_info[*tag_count], sizeof(struct gooseberry_tag_info));
        }
    } while ((*tag_count < tag_limit) && (gooseberry_try_read_next_tag(&context_copy, &current_tag) == 1));
    
    return (*tag_count > 0)  ? 1 : 0;
}

// Are the next N tags of a specific type and/or size?
// If they aren't N tags ahead, 0 is returned.
uint8_t gooseberry_are_next_tags(struct gooseberry_context* context, struct gooseberry_tag_info* tag_info, uintptr tag_count, uint8_t match_type, uint8_t match_size)
{
    struct gooseberry_context context_copy = { 0 };
    gooseberry_memcpy((void*)context, (void*)&context_copy, sizeof(context_copy));

    struct gooseberry_tag_info current_tag = { 0 };

    if(!gooseberry_try_read_next_tag(&context_copy, &current_tag))
    {
        return 0;
    }

    if(match_type)
    {
        if(current_tag.tag.bits.type != tag_info[0].tag.bits.type)
        {
            return 0;
        }
    }

    if(match_size)
    {
        if(current_tag.tag.bits.size != tag_info[0].tag.bits.size)
        {
            return 0;
        }
    }

    for(uintptr i = 1; i < tag_count; i++)
    {
        if(!gooseberry_try_read_next_tag(&context_copy, &current_tag))
        {
            return 0;
        }

        if(match_type)
        {
            if(current_tag.tag.bits.type != tag_info[i].tag.bits.type)
            {
                return 0;
            }
        }

        if(match_size)
        {
            if(current_tag.tag.bits.size != tag_info[i].tag.bits.size)
            {
                return 0;
            }
        }
    }

    return 1;
}

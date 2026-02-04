#include "analyzer.hpp"

#include <cstring>

hashtable::hashtable(uint32_t size) : size(size), entries(new hashtable_entry[size]())
{
    for (uint32_t i = 0; i < size; i++)
    {
        entries[i].key.length = 0;
    }
}

bool equals(uint8_t* code_ptr, hashtable_key const& key, uint32_t ip, uint32_t length)
{
    if (key.length != length)
    {
        return false;
    }
    for (uint32_t i = 0; i < length; ++i)
    {
        if (code_ptr[key.ip + i] != code_ptr[ip + i])
        {
            return false;
        }
    }
    return true;
}

static hashtable_entry&
get_entry(hashtable& hashtable, uint8_t* code_ptr, uint32_t hash, uint32_t ip, uint32_t length)
{
    uint32_t index = (hash * mixing_constant) % hashtable.size;
    while (true)
    {
        if (hashtable.entries[index].key.length == 0)
        {
            return hashtable.entries[index];
        }
        if (equals(code_ptr, hashtable.entries[index].key, ip, length))
        {
            return hashtable.entries[index];
        }
        index = (index + 1) % hashtable.size;
    }
}

void hashtable::mark_occurrence(uint8_t* code_ptr, uint32_t hash, uint32_t ip, uint32_t length)
{
    hashtable_entry& entry = get_entry(*this, code_ptr, hash, ip, length);
    if (entry.key.length)
    {
        entry.value++;
    }
    else
    {
        entry.key.ip = ip;
        entry.key.length = length;
        entry.value = 1;
    }
}

uint32_t hashtable::pack()
{
    uint32_t packed_pointer = 0;
    uint32_t unpacked_pointer = 0;

    while (unpacked_pointer < size)
    {
        hashtable_entry& packed_entry = entries[packed_pointer];
        hashtable_entry& unpacked_entry = entries[unpacked_pointer];

        if (packed_entry.key.length == 0 && unpacked_entry.key.length != 0)
        {
            packed_entry = unpacked_entry;
            unpacked_entry.key.length = 0;
            packed_pointer++;
            unpacked_pointer++;
        }
        else if (packed_entry.key.length == 0)
        {
            unpacked_pointer++;
        }
        else
        {
            packed_pointer++;
            unpacked_pointer++;
        }
    }

    return packed_pointer;
}

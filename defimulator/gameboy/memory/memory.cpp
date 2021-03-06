#include <gameboy/gameboy.hpp>

#define MEMORY_CPP
namespace GameBoy {

Unmapped unmapped;
Bus bus;

uint8_t& Memory::operator[](unsigned addr)
{
    return data[addr];
}

void Memory::allocate(unsigned size_)
{
    free();
    size = size_;
    data = new uint8_t[size]();
}

void Memory::copy(const uint8_t *data_, unsigned size_)
{
    free();
    size = size_;
    data = new uint8_t[size];
    memcpy(data, data_, size);
}

void Memory::free(void)
{
    if (data) {
        delete[](data);
        data = 0;
    }
}

Memory::Memory(void)
{
    data = 0;
    size = 0;
}

Memory::~Memory(void)
{
    free();
}

uint8 Bus::read(uint16 addr)
{
    return mmio[addr]->mmio_read(addr);
}

void Bus::write(uint16 addr, uint8 data)
{
    mmio[addr]->mmio_write(addr, data);
}

void Bus::power(void)
{
    for (unsigned n = 0; n < 65536; n++) {
        mmio[n] = &unmapped;
    }
}

}

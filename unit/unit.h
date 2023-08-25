#ifndef UNIT_H
#define UNIT_H

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <time.h>
#include <functional>
#include <iostream>
#include <unistd.h>
#include <mpi.h>

class Unit
{
public:
    ~Unit() {}

    const int getRank() { return m_rank; }
    const std::size_t getBufferBytes() const { return m_bufferBytes; }
    int8_t *getBuffer() const { return m_buffer; }

protected:
    void allocateMemory();

    int m_rank;

    typedef std::unique_ptr<void, std::function<void(void *)>> buffer_t;

    buffer_t m_memBufferPtr;
    std::size_t m_bufferBytes = 1e7;
    int8_t *m_buffer;
};

#endif // UNIT_H
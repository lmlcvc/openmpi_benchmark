#include "unit.h"

Unit::Unit(int rank) {
    m_rank = rank;
}

void Unit::allocateMemory()
{
    const long pageSize = sysconf(_SC_PAGESIZE);

    void *mem = nullptr;

    if (posix_memalign(&mem, pageSize, m_bufferBytes) != 0)
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    m_buffer = static_cast<int8_t *>(mem);
    std::fill(m_buffer, m_buffer + m_bufferBytes, 0);

    m_memBufferPtr = buffer_t(mem, free);
}

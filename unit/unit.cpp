#include "unit.h"

Unit::Unit()
{
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    m_shift = std::make_unique<std::vector<int>>(size / 2);
    std::iota(m_shift->begin(), m_shift->end(), 0);
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

void Unit::ruShift(int idx)
{
    std::rotate(m_shift->begin(), m_shift->begin() + idx, m_shift->end());
}

void Unit::buShift(int idx)
{
    std::vector<int> vec_r(m_shift->size());
    std::copy(m_shift->rbegin(), m_shift->rend(), vec_r.begin());

    std::copy(vec_r.begin(), vec_r.end(), m_shift->begin());
    std::rotate(m_shift->begin(), m_shift->end() - 1 - idx, m_shift->end());
}
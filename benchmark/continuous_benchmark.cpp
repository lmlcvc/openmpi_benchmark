#include "continuous_benchmark.h"

void ContinuousBenchmark::allocateMemory()
{
    const long pageSize = sysconf(_SC_PAGESIZE);

    m_bufferSnd = static_cast<int8_t *>(memSnd);
    std::fill(m_bufferSnd, m_bufferSnd + m_sndBufferBytes, 0);

    m_bufferRcv = static_cast<int8_t *>(memRcv);

    if (posix_memalign(&memSnd, pageSize, m_sndBufferBytes) != 0 && posix_memalign(&memRcv, pageSize, m_rcvBufferBytes))
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    MPI_Bcast(&memSnd, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memSndPtr(m_rank == 0 ? memSnd : nullptr, &free);

    MPI_Bcast(&memRcv, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memRcvPtr(m_rank == 0 ? memRcv : nullptr, &free);
}

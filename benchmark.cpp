#include "benchmark.h"

timespec Benchmark::diff(timespec start, timespec end)
{
    timespec time_diff;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        time_diff.tv_sec = end.tv_sec - start.tv_sec - 1;
        time_diff.tv_nsec = 1e9 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        time_diff.tv_sec = end.tv_sec - start.tv_sec;
        time_diff.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return time_diff;
}

void Benchmark::setup(int rank)
{
    // Implementation for setup
}

void Benchmark::run()
{
    // Implementation for run
}

void Benchmark::allocateMemory(int rank)
{
    bufferSnd = static_cast<int8_t *>(memSnd);
    bufferRcv = static_cast<int8_t *>(memRcv);
    std::fill(bufferSnd, bufferSnd + m_alignSize, 0);

    MPI_Bcast(&memSnd, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memSndPtr(rank == 0 ? memSnd : nullptr, &free);

    MPI_Bcast(&memRcv, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memRcvPtr(rank == 0 ? memRcv : nullptr, &free);
}
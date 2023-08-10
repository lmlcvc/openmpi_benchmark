#include "continuous_benchmark.h"

ContinuousBenchmark::ContinuousBenchmark(int argc, char **argv, int rank) : Benchmark(argc, argv, rank)
{
    m_rank = rank;
}

void ContinuousBenchmark::setup(int rank)
{
    m_alignSize = m_messageSize;
    allocateMemory(rank);

    // TODO: init vector of random message sizes
}

void ContinuousBenchmark::run()
{
}

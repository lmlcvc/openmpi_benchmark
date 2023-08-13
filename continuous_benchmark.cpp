#include "continuous_benchmark.h"

ContinuousBenchmark::ContinuousBenchmark(int argc, char **argv, int rank)
{
    m_rank = rank;
}

void ContinuousBenchmark::performWarmup()
{
   
}

void ContinuousBenchmark::setup()
{
    m_alignSize = m_messageSize;
    allocateMemory();

    // TODO: init vector of random message sizes
}

void ContinuousBenchmark::run()
{
}

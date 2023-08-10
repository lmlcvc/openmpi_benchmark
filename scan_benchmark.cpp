#include "scan_benchmark.h"

ScanBenchmark::ScanBenchmark(int argc, char **argv, int rank) : Benchmark(argc, argv, rank)
{
    m_rank = rank;
}


void ScanBenchmark::setup(int rank)
{
    m_alignSize = static_cast<std::size_t>(std::pow(2, maxPower));
    allocateMemory(rank);
}

void ScanBenchmark::run()
{
}

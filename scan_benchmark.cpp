#include "scan_benchmark.h"

ScanBenchmark::ScanBenchmark(int argc, char **argv, int rank)
{
    m_rank = rank;
}

void ScanBenchmark::performWarmup()
{
    // TODO: handle case when send buffer is smaller than whatever is used here
    // TODO: handle cases for variable message sizes
    // maybe use new buffer

    timespec startTime, endTime;
    double throughput;
    std::size_t messageSize = 1e6;
    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(m_sndBufferSize * m_alignSize); // FIXME: should be whatever the buffer size is

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    rtCommunication(m_sndBufferSize, m_rcvBufferSize, messageSize, m_warmupIterations);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, messageSize, m_warmupIterations);

    std::cout << "\nPre-warmup throughput: " << throughput << " Mbit/s" << std::endl;

    warmupCommunication(m_bufferSnd, subarrayIndices, m_rank);

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    rtCommunication(m_sndBufferSize, m_rcvBufferSize, messageSize, m_warmupIterations);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, messageSize, m_warmupIterations);

    std::cout << "Post-warmup throughput: " << throughput << " Mbit/s\n"
              << std::endl;
}

void ScanBenchmark::setup() 
{
    m_alignSize = static_cast<std::size_t>(std::pow(2, maxPower)); // FIXME: should be full size of buffer
    allocateMemory();
}

void ScanBenchmark::run()
{
}

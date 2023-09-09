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

std::vector<std::pair<int, int>> Benchmark::findSubarrayIndices(std::size_t messageSize)
{
    int subarraySize = std::ceil(0.32 * messageSize);
    int sharedElements = std::ceil(0.15 * messageSize);

    std::vector<std::pair<int, int>> subarrayIndices;

    for (int i = 0; i < 5; ++i)
    {
        int start = i * (subarraySize - sharedElements);
        int end = start + subarraySize;
        subarrayIndices.push_back(std::make_pair(start, end));
    }

    return subarrayIndices;
}

std::pair<double, double> Benchmark::calculateThroughput(timespec startTime, timespec endTime, std::size_t bytesTransferred, std::size_t iterations)
{
    timespec elapsedTime = diff(startTime, endTime);
    double elapsedSecs = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    double avgRtt = elapsedSecs / iterations;
    double avgThroughput = (bytesTransferred * 8.0) / (elapsedSecs * 1e6);

    return std::make_pair(avgRtt, avgThroughput);
}

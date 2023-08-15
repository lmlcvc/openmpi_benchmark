#include "benchmark.h"

timespec diff(timespec start, timespec end)
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
    // TODO: override for ContinuousVariableMessage
    // XXX: don't need iterations for scan run
    timespec elapsedTime = diff(startTime, endTime);
    double elapsedSecs = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    double avgRtt = elapsedSecs / iterations;
    double avgThroughput = (bytesTransferred * 8.0) / (elapsedSecs * 1e6);

    return std::make_pair(avgRtt, avgThroughput);
}

void Benchmark::printRunInfo(double rtt, double throughput, int errorMessagesCount)
{
    if (m_rank)
        return;

    std::cout << std::fixed << std::setprecision(8);

    std::cout << std::right << std::setw(14) << " Avg. RTT"
              << " | " << std::setw(25) << "Throughput"
              << " | " << std::setw(10) << " Errors"
              << std::endl;

    std::cout << std::right << std::setw(12) << rtt << " s"
              << " | " << std::setw(18) << std::fixed << std::setprecision(2) << throughput << " Mbit/s"
              << " | " << std::setw(10) << errorMessagesCount << std::endl
              << std::endl;
}

std::size_t Benchmark::rtCommunication(std::size_t messageSize, std::size_t iterations = -1)
{
    if (iterations == -1)
        iterations = m_iterations;

    std::vector<MPI_Status> statuses(iterations);
    std::size_t sendOffset = 0, recvOffset = 0;
    std::size_t remainingSize, wrapSize;

    if (m_rank == 0)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            if (sendOffset + messageSize > m_sndBufferBytes)
            {
                remainingSize = m_sndBufferBytes - sendOffset;
                wrapSize = messageSize - remainingSize;

                MPI_Send(m_bufferSnd + sendOffset, remainingSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                sendOffset = 0;
                MPI_Send(m_bufferSnd + sendOffset, wrapSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            }
            else
                MPI_Send(m_bufferSnd + sendOffset, messageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);

            sendOffset = (sendOffset + messageSize) % m_sndBufferBytes;
        }
    }
    else if (m_rank == 1)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            MPI_Status status1, status2;

            if (recvOffset + messageSize > m_rcvBufferBytes)
            {
                remainingSize = m_rcvBufferBytes - recvOffset;
                wrapSize = messageSize - remainingSize;

                MPI_Recv(m_bufferRcv + recvOffset, remainingSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status1);
                recvOffset = 0;
                MPI_Recv(m_bufferRcv + recvOffset, wrapSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status2);

                if (status1.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status1.MPI_ERROR;
                else if (status2.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status2.MPI_ERROR;
            }
            else
                MPI_Recv(m_bufferRcv + recvOffset, messageSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &statuses[i]);

            recvOffset = (recvOffset + messageSize) % m_rcvBufferBytes;
        }
    }

    return std::count_if(statuses.begin(), statuses.end(),
                         [](const MPI_Status &status)
                         { return status.MPI_ERROR != MPI_SUCCESS; });
}

void Benchmark::warmupCommunication(int8_t *bufferSnd, std::vector<std::pair<int, int>> subarrayIndices, int8_t rank)
{
    std::size_t subarrayCount = subarrayIndices.size();
    std::size_t subarraySize;

    if (rank == 0)
        std::cout << "Performing warmup...";

    if (rank == 0)
    {
        for (int i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Send(bufferSnd + subarrayIndices[i].first, subarraySize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        }
    }
    else if (rank == 1)
    {
        for (int i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Recv(bufferSnd + subarrayIndices[i].first, subarraySize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    if (rank == 0)
        std::cout << "Done." << std::endl;
}

void Benchmark::performWarmup()
{
    timespec startTime, endTime;
    double throughput;
    std::size_t messageSize = m_sndBufferBytes / 10;

    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(m_sndBufferBytes);

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    rtCommunication(messageSize, m_warmupIterations);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, messageSize * m_warmupIterations, m_warmupIterations);

    std::cout << "\nPre-warmup throughput: " << throughput << " Mbit/s" << std::endl;

    warmupCommunication(m_bufferSnd, subarrayIndices, m_rank);

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    rtCommunication(messageSize, m_warmupIterations);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, messageSize * m_warmupIterations, m_warmupIterations);

    std::cout << "Post-warmup throughput: " << throughput << " Mbit/s\n"
              << std::endl;
}

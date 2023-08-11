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

void Benchmark::allocateMemory(int rank)
{
    const long pageSize = sysconf(_SC_PAGESIZE);
    m_bufferSnd = static_cast<int8_t *>(memSnd);
    m_bufferRcv = static_cast<int8_t *>(memRcv);
    std::fill(m_bufferSnd, m_bufferRcv + m_alignSize * m_sndBufferSize, 0);

    // TODO: align both buffers with respective align size
    if (posix_memalign(&memSnd, pageSize, m_alignSize) != 0 && posix_memalign(&memRcv, pageSize, m_alignSize))
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
    std:
        exit(1);
    }

    MPI_Bcast(&memSnd, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memSndPtr(rank == 0 ? memSnd : nullptr, &free);

    MPI_Bcast(&memRcv, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memRcvPtr(rank == 0 ? memRcv : nullptr, &free);
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

std::pair<double, double> Benchmark::calculateThroughput(timespec startTime, timespec endTime, std::size_t messageSize, std::size_t iterations)
{
    // TODO: for variable size, it should be total messages size
    timespec elapsedTime = diff(startTime, endTime);
    double elapsedSecs = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    double avgRtt = elapsedSecs / iterations;
    double avgThroughput = (iterations * messageSize * 8.0) / (elapsedSecs * 1e6);

    return std::make_pair(avgRtt, avgThroughput);
}

std::size_t Benchmark::rtCommunication(std::size_t sndBufferSize, std::size_t rcvBufferSize, std::size_t messageSize, std::size_t iterations = -1)
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
            if (sendOffset + messageSize > sndBufferSize * messageSize)
            {
                remainingSize = sndBufferSize - sendOffset;
                wrapSize = messageSize - remainingSize;

                MPI_Send(m_bufferSnd + sendOffset, remainingSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                sendOffset = 0;
                MPI_Send(m_bufferSnd + sendOffset, wrapSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            }
            else
                MPI_Send(m_bufferSnd + sendOffset, messageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);

            sendOffset = (sendOffset + messageSize) % sndBufferSize;
        }
    }
    else if (m_rank == 1)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            MPI_Status status1, status2;

            if (recvOffset + messageSize > rcvBufferSize * messageSize)
            {
                remainingSize = rcvBufferSize - recvOffset;
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

            recvOffset = (recvOffset + messageSize) % rcvBufferSize;
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

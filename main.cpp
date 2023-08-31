#include <iostream>
#include <cstddef>
#include <csignal>
#include <vector>
#include <string>
#include <memory>
#include <mpi.h>

#include "benchmark/benchmark.h"
#include "benchmark/scan_benchmark.h"
#include "benchmark/continuous_benchmark.h"
#include "benchmark/variable_message.h"
#include "benchmark/fixed_message.h"

volatile sig_atomic_t sigintReceived = 0;

// TODO: run arguments (define and adapt code)
// FRAG_SIZE
// CHUNK_SIZE - if none, do not split message ( = FRAG_SIZE )

void handleSignals(int signal)
{
    if (signal == SIGINT)
        sigintReceived = 1;
}

void printHelp()
{
    std::cout << "Modes of Operation:\n";
    std::cout << "  -s            Perform a scan run.\n";
    std::cout << "  -f            Perform a fixed message size run.\n";
    std::cout << "  -v            Perform a variable message size run.\n";
    std::cout << "  -n            Use non-blocking mode.\n\n";

    std::cout << "  SCAN RUN:\n";
    std::cout << "    -s                      Perform a scan run with varied message sizes.\n";
    std::cout << "    -p <max power>           Set the maximum power of 2 for message sizes.\n";
    std::cout << "    -i <iterations>          Specify the number of iterations.\n";
    std::cout << "    -b <send buffer size>    Set the size of the send buffer in messages.\n";
    std::cout << "    -r <receive buffer size> Set the size of the receive buffer in messages.\n";
    std::cout << "    -w <warmup iterations>   Set the number of warmup iterations.\n\n";

    std::cout << "  FIXED MESSAGE SIZE RUN:\n";
    std::cout << "    -f                      Perform a run with a fixed message size.\n";
    std::cout << "    -m <message size>        Set the fixed message size.\n";
    std::cout << "    -i <iterations>          Specify the number of iterations.\n";
    std::cout << "    -r <RU buffer bytes>     Set the size of the send buffer in bytes.\n";
    std::cout << "    -b <BU buffer bytes>     Set the size of the receive buffer in bytes.\n";
    std::cout << "    -w <warmup iterations>   Set the number of warmup iterations.\n\n";

    std::cout << "  VARIABLE MESSAGE SIZE RUN:\n";
    std::cout << "    -v                         Perform a run with variable message sizes.\n";
    std::cout << "    -m <message size variants> Set the number of message size variants.\n";
    std::cout << "    -i <iterations>            Specify the number of iterations.\n";
    std::cout << "    -r <RU buffer bytes>       Set the size of the send buffer in bytes.\n";
    std::cout << "    -b <BU buffer bytes>       Set the size of the receive buffer in bytes.\n";
    std::cout << "    -w <warmup iterations>     Set the number of warmup iterations.\n";
}

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

void printElapsed(timespec runStartTime)
{
    timespec runEndTime;
    clock_gettime(CLOCK_MONOTONIC, &runEndTime);
    timespec elapsedTime = diff(runStartTime, runEndTime);
    double elapsedSeconds = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    std::cout << "Finishing program. Total elapsed time: " << elapsedSeconds << " s" << std::endl;
}

void parseArguments(int argc, char **argv, int rank, CommunicationType &commType, std::vector<ArgumentEntry> &commArguments)
{
    int opt;
    bool nonblocking = false;
    while ((opt = getopt(argc, argv, "m:i:b:w:sfvr:nh")) != -1)
    {
        switch (opt)
        {
        case 's':
            if (commType == COMM_UNDEFINED)
            {
                commType = COMM_SCAN;
            }
            else if (rank == 0)
            {
                std::cerr << "Cannot have multiple communication types" << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
            break;
        case 'f':
            if (commType == COMM_UNDEFINED)
            {
                commType = nonblocking ? COMM_FIXED_NONBLOCKING : COMM_FIXED_BLOCKING;
            }
            else if (rank == 0)
            {
                std::cerr << "Cannot have multiple communication types" << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
            break;
        case 'v':
            if (commType == COMM_UNDEFINED)
            {
                commType = COMM_VARIABLE_BLOCKING;
            }
            else if (rank == 0)
            {
                std::cerr << "Cannot have multiple communication types" << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
            break;
        case 'n':
            nonblocking = true;
            break;
        case 'm':
        case 'i':
        case 'b':
        case 'r':
        case 'w':
        case 'p':
            commArguments.push_back({static_cast<char>(opt), optarg});
            break;
        case 'h':
        default:
            if (rank == 0)
                printHelp();
            MPI_Finalize();
            std::exit(1);
        }
    }

    // If -n flag is present, update the communication type
    if (nonblocking)
    {
        if (commType == COMM_VARIABLE_BLOCKING)
            commType = COMM_VARIABLE_NONBLOCKING;
        if (commType == COMM_FIXED_BLOCKING)
            commType = COMM_FIXED_NONBLOCKING;
    }
}

int main(int argc, char **argv)
{
    int rank, size;
    CommunicationType commType = COMM_UNDEFINED;
    std::vector<ArgumentEntry> commArguments;

    std::unique_ptr<Benchmark> benchmark;

    bool continueRun = true;
    timespec runStartTime;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    // Benchmark object initialisation
    parseArguments(argc, argv, rank, commType, commArguments);
    if (commType == COMM_SCAN)
    {
        benchmark = std::make_unique<ScanBenchmark>(commArguments, rank);
        continueRun = false;
    }
    else if (commType == COMM_FIXED_BLOCKING || commType == COMM_FIXED_NONBLOCKING)
    {
        benchmark = std::make_unique<BenchmarkFixedMessage>(commArguments, rank, size, commType);
    }

    else if (commType == COMM_VARIABLE_BLOCKING || commType == COMM_VARIABLE_NONBLOCKING)
    {
        benchmark = std::make_unique<BenchmarkVariableMessage>(commArguments, rank, size, commType);
    }
    else
    {
        if (rank == 0)
            std::cerr << "Invalid communication type. Finishing." << std::endl;
        MPI_Finalize();
        return 1;
    }

    std::signal(SIGINT, [](int signal)
                { handleSignals(signal); });

    // Run program
    clock_gettime(CLOCK_MONOTONIC, &runStartTime);
    benchmark->performWarmup();
    do
    {
        benchmark->run();

        if (sigintReceived)
        {
            if (rank == 0)
                printElapsed(runStartTime);
            MPI_Finalize();
            std::exit(EXIT_SUCCESS);
        }
    } while (continueRun);

    // Finalise program
    MPI_Finalize();

    return 0;
}

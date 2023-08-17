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
    std::cout << "    -b <send buffer size>   Set the size of the send buffer in messages.\n";
    std::cout << "    -r <receive buffer size> Set the size of the receive buffer in messages.\n";
    std::cout << "    -w <warmup iterations>   Set the number of warmup iterations.\n\n";

    std::cout << "  FIXED MESSAGE SIZE RUN:\n";
    std::cout << "    -f                      Perform a run with a fixed message size.\n";
    std::cout << "    -m <message size>        Set the fixed message size.\n";
    std::cout << "    -i <iterations>          Specify the number of iterations.\n";
    std::cout << "    -b <send buffer bytes>   Set the size of the send buffer in bytes.\n";
    std::cout << "    -r <receive buffer bytes> Set the size of the receive buffer in bytes.\n";
    std::cout << "    -w <warmup iterations>   Set the number of warmup iterations.\n\n";

    std::cout << "  VARIABLE MESSAGE SIZE RUN:\n";
    std::cout << "    -v                      Perform a run with variable message sizes.\n";
    std::cout << "    -m <message size variants> Set the number of message size variants.\n";
    std::cout << "    -i <iterations>          Specify the number of iterations.\n";
    std::cout << "    -b <send buffer bytes>   Set the size of the send buffer in bytes.\n";
    std::cout << "    -r <receive buffer bytes> Set the size of the receive buffer in bytes.\n";
    std::cout << "    -w <warmup iterations>   Set the number of warmup iterations.\n";
}

void parseArguments(int argc, char **argv, int rank, CommunicationType &commType, std::vector<ArgumentEntry> &commArguments)
{
    int opt;
    bool nonblocking = false;
    while ((opt = getopt(argc, argv, "m:i:b:w:sfvnh")) != -1)
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
                commType = nonblocking ? COMM_VARIABLE_NONBLOCKING : COMM_VARIABLE_BLOCKING;
            }
            else if (rank == 0)
            {
                std::cerr << "Cannot have multiple communication types" << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
            break;
        case 'n':
            if (commType == COMM_VARIABLE_BLOCKING)
                commType = COMM_VARIABLE_NONBLOCKING;
            if (commType == COMM_FIXED_BLOCKING)
                commType = COMM_FIXED_NONBLOCKING;
            nonblocking = true;
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
}

int main(int argc, char **argv)
{
    int rank, size;
    CommunicationType commType = COMM_UNDEFINED;
    std::vector<ArgumentEntry> commArguments;

    std::unique_ptr<Benchmark> benchmark;

    std::signal(SIGINT, handleSignals);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Run setup
    std::cout << "Initialised rank " << rank << std::endl;

    if (size != 2)
    {
        std::cerr << "This benchmark requires exactly 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    // Benchmark object initialisation
    parseArguments(argc, argv, rank, commType, commArguments);
    if (commType == COMM_SCAN)
    {
        benchmark = std::make_unique<ScanBenchmark>(commArguments, rank);
    }
    else if (commType == COMM_FIXED_BLOCKING)
    {
        benchmark = std::make_unique<BenchmarkFixedMessage>(commArguments, rank, commType);
    }
    else if (commType == COMM_VARIABLE_BLOCKING)
    {
        benchmark = std::make_unique<BenchmarkVariableMessage>(commArguments, rank, commType);
    }
    else
    {
        if (rank == 0)
            std::cerr << "Invalid communication type. Finishing." << std::endl;
        MPI_Finalize();
        return 1;
    }

    // Run program
    benchmark->run();

    // Finalise program
    MPI_Finalize();

    return 0;
}

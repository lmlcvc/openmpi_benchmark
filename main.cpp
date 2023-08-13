#include <iostream>
#include <cstddef>
#include <csignal>
#include <vector>
#include <string>
#include <memory>
#include <mpi.h>

#include "benchmark.h"
#include "scan_benchmark.h"
#include "continuous_benchmark.h"

// FIXME: main hangs

volatile sig_atomic_t sigintReceived = 0;
timespec run_start_time;

enum CommunicationType
{
    COMM_UNDEFINED,
    COMM_SCAN = 's',
    COMM_CONTINUOUS = 'c'
};

void handleSignals(int signal)
{
    if (signal == SIGINT)
        sigintReceived = 1;
}

void parseArguments(int argc, char **argv, int rank, CommunicationType &commType, std::vector<ArgumentEntry> &commArguments)
{
    int opt;
    while ((opt = getopt(argc, argv, "m:i:b:w:sch")) != -1)
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
        case 'c':
            if (commType == COMM_UNDEFINED)
            {
                commType = COMM_CONTINUOUS;
            }
            else if (rank == 0)
            {
                std::cerr << "Cannot have multiple communication types" << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
            break;
        case 'm':
        case 'i':
        case 'b':
        case 'w':
            commArguments.push_back({static_cast<char>(opt), optarg});
            break;
        case 'h':
        default:
            if (rank == 0)
                std::cout << "Usage: " << argv[0] << std::endl
                          << "\t-m <message-size>\n\t-i <print-interval>\n\t"
                          << "-b <buffer-size> [messages]\n\t-w <iterations in warmup throughput>\n\t"
                          << "-c continuous run\n\t-s perform scan\n\t-h help" << std::endl;
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
    else if (commType == COMM_CONTINUOUS)
    {
        benchmark = std::make_unique<ContinuousBenchmark>(commArguments, rank);
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

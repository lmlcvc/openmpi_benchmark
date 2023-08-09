#include <iostream>
#include <cstddef>
#include <csignal>
#include <vector>
#include <string>
#include <mpi.h>

volatile sig_atomic_t sigintReceived = 0; // indicate if SIGINT has been received
timespec run_start_time;

enum CommunicationType
{
    COMM_UNDEFINED,
    COMM_SCAN = 's',
    COMM_CONTINUOUS = 'c'
};

struct ArgumentEntry {
    char option;
    std::string value;
};

void handleSignals(int signal)
{
    if (signal == SIGINT)
        sigintReceived = 1;
}

void parseArguments(int argc, char **argv, int rank, CommunicationType &commType, std::vector<ArgumentEntry> &commArguments)
{
    int opt;
    while ((opt = getopt(argc, argv, "m:i:c:b:w:sh")) != -1)
    {
        switch (opt)
        {
        case 's':
            commType = COMM_SCAN;
            commArguments.push_back({ 's', "" });
            break;
        case 'c':
            commType = COMM_CONTINUOUS;
            commArguments.push_back({ 'c', "" });
            break;
        case 'm':
        case 'i':
        case 'b':
        case 'w':
            commArguments.push_back({ static_cast<char>(opt), optarg });
            break;
        case 'h':
        default:
            if (rank == 0)
                std::cout << "Usage: " << argv[0] << std::endl
                          << "\t-m <message-size>\n\t-i <print-interval>\n\t"
                          << "-b <messages in buffer>\n\t-w <iterations in warmup throughput>\n\t"
                          << "-s perform scan\n\t-h help" << std::endl;
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

    std::signal(SIGINT, handleSignals);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Initialised rank " << rank << std::endl;

    if (size != 2)
    {
        std::cerr << "This benchmark requires exactly 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    parseArguments(argc, argv, rank, commType, commArguments);

    // INIT DESIRED TYPE OF OBJECT

    // ALLOCATE MEMORY

    // Initialise message and data
    // TODO: replace with variable size message/buffer initialisation

    // Perform warmup

    // Run program

    // Finalise program
    MPI_Finalize();

    return 0;
}

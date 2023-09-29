#include <iostream>
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <mpi.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>

#include "benchmark/benchmark.h"
#include "benchmark/scan_benchmark.h"
#include "benchmark/continuous_benchmark.h"
#include "benchmark/variable_message.h"
#include "benchmark/fixed_message.h"

void printHelp()
{
    std::cout << "Modes of Operation:\n";
    std::cout << "  Scan run (-mode scan)\n";
    std::cout << "  Fixed message size run (-mode fixed)\n";
    std::cout << "  Variable message size run (-mode variable)\n";
    std::cout << "  Use non-blocking mode (-n).\n\n";

    std::cout << "  SCAN RUN:\n";
    std::cout << "    <max power>           Set the maximum power of 2 for message sizes.\n";
    std::cout << "    <iterations>          Specify the number of iterations.\n";
    std::cout << "    <send buffer size>    Set the size of the send buffer in messages.\n";
    std::cout << "    <receive buffer size> Set the size of the receive buffer in messages.\n";
    std::cout << "    <warmup iterations>   Set the number of warmup iterations.\n\n";

    std::cout << "  FIXED MESSAGE SIZE RUN:\n";
    std::cout << "    <message size>        Set the fixed message size.\n";
    std::cout << "    <msgs per phase>      Set the number of messages in each phase.\n";
    std::cout << "    <iterations>          Specify the number of iterations.\n";
    std::cout << "    <RU buffer bytes>     Set the size of the send buffer in bytes.\n";
    std::cout << "    <BU buffer bytes>     Set the size of the receive buffer in bytes.\n";
    std::cout << "    <warmup iterations>   Set the number of warmup iterations.\n";
    std::cout << "    <logging interval>    Set the interval for average throughput logging in seconds.\n";
    std::cout << "    <config path>         Configuration json with info on the hosts.\n\n";

    std::cout << "  VARIABLE MESSAGE SIZE RUN:\n";
    std::cout << "    <message size variants> Set the number of message size variants.\n";
    std::cout << "    <msgs per phase>        Set the number of messages in each phase.\n";
    std::cout << "    <iterations>            Specify the number of iterations.\n";
    std::cout << "    <RU buffer bytes>       Set the size of the send buffer in bytes.\n";
    std::cout << "    <BU buffer bytes>       Set the size of the receive buffer in bytes.\n";
    std::cout << "    <warmup iterations>     Set the number of warmup iterations.\n";
    std::cout << "    <logging interval>      Set the interval for average throughput logging in seconds.\n";
    std::cout << "    <config path>           Configuration json with info on the hosts.\n\n";
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

std::string getCurrentDate()
{
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

bool directoryExists(const std::string &path)
{
    struct stat info;
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

/**
 * @brief Run arguments parsing for benchmark initialisation.
 *
 * Parses benchmark type-related arguments, pushes the rest to be parsed in benchmark object.
 *
 * @param argc
 * @param argv
 * @param rank Process rank (for printing)
 * @param commType Type of benchmark run
 * @param commArguments Args not related to benchmark type, passed to benchmark obj for parsing
 *
 */
void parseArguments(int argc, char **argv, int rank, CommunicationType &commType, std::vector<ArgumentEntry> &commArguments)
{
    int opt;
    bool nonblocking = false;
    while ((opt = getopt(argc, argv, "m:i:b:w:sfvr:l:c:p:nh")) != -1)
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
        case 'l':
        case 'c':
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

/**
 * @brief Create path for log file
 *
 * Initialises base directory and date-related log file within it (if needed).
 *
 * @param baseDirectory Directory indicating type of log
 * @param rank Process rank (can only be created by one process)
 * @return const std::string logFilepath
 */
const std::string createLogFilepath(std::string baseDirectory, int rank)
{
    std::string parentDirectory = "logs";
    std::string logDirFilepath = parentDirectory + "/" + baseDirectory;
    std::string logFilepath = logDirFilepath + "/" + getCurrentDate() + ".csv";

    if (rank == 0)
    {
        if (!directoryExists(parentDirectory))
        {
            if (mkdir(parentDirectory.c_str(), 0777) != 0)
            {
                std::cerr << "Error creating parent directory: " << parentDirectory << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
        }

        if (!directoryExists(logDirFilepath))
        {
            if (mkdir(logDirFilepath.c_str(), 0777) != 0)
            {
                std::cerr << "Error creating directory: " << logDirFilepath << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
        }
    }

    return logFilepath;
}

int main(int argc, char **argv)
{
    int rank, size;
    char hostname[32];
    CommunicationType commType = COMM_UNDEFINED;
    std::vector<ArgumentEntry> commArguments;

    std::unique_ptr<Benchmark> benchmark;

    bool continueRun = true;
    timespec runStartTime;

    // MPI setup
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    gethostname(hostname, sizeof(hostname));
    std::string host_str(hostname);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    std::cout << "Rank " << rank << " initialised on " << host_str << std::endl;

    // Benchmark object initialisation
    parseArguments(argc, argv, rank, commType, commArguments);
    if (commType == COMM_SCAN)
    {
        benchmark = std::make_unique<ScanBenchmark>(commArguments);
        continueRun = false;
    }
    else if (commType == COMM_FIXED_BLOCKING || commType == COMM_FIXED_NONBLOCKING)
    {
        benchmark = std::make_unique<BenchmarkFixedMessage>(commArguments, commType);
    }

    else if (commType == COMM_VARIABLE_BLOCKING || commType == COMM_VARIABLE_NONBLOCKING)
    {
        benchmark = std::make_unique<BenchmarkVariableMessage>(commArguments, commType);
    }
    else
    {
        if (rank == 0)
            std::cerr << "Invalid communication type. Finishing." << std::endl;
        MPI_Finalize();
        return 1;
    }

    benchmark->setPhasesFilepath(createLogFilepath("phases", rank));
    benchmark->setAvgThroughputFilepath(createLogFilepath("avg_throughput", rank));

    // Run program
    clock_gettime(CLOCK_MONOTONIC, &runStartTime);
    benchmark->performWarmup();

    do
    {
        benchmark->run();
    } while (continueRun);

    MPI_Finalize();

    return 0;
}

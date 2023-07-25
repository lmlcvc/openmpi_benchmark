#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <unistd.h>
#include <time.h>
#include <mpi.h>
#include <chrono>
#include <thread>
#include <cmath>

// Function to calculate the difference between two timespec structures
timespec diff(timespec start, timespec end)
{
    timespec time_diff;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        time_diff.tv_sec = end.tv_sec - start.tv_sec - 1;
        time_diff.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        time_diff.tv_sec = end.tv_sec - start.tv_sec;
        time_diff.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return time_diff;
}


// Functions for formatted throughput info printing
void print_continuous(std::size_t round_trip, double rtt, double throughput)
{
    std::cout << std::fixed << std::setprecision(8);

    std::cout << "| " << std::left << std::setw(12) << "Round Trip"
                << " | " << std::right << std::setw(14) << "RTT"
                << " | " << std::setw(25) << "Throughput"
                << " |" << std::endl;

    std::cout << "| " << std::left << std::setw(12) << round_trip
                << " | " << std::right << std::setw(12) << rtt << " s"
                << " | " << std::setw(18) << throughput << " Mbit/s"
                << " |\n"
                << std::endl;
}

void print_discrete(std::size_t message_size, double throughput)
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "| " << std::left << std::setw(12) << message_size
                        << " | " << std::setw(19) << throughput
                        << " |\n";
}

// Function to perform round-trip communication between two processes
void perform_rt_communication(int8_t *message, std::size_t message_size, std::size_t round_trip,
                              int8_t rank, bool print_enabled = false, bool continuous_send = true)
{
    if (rank == 0)
    {
        timespec rt_start_time;
        if (print_enabled)
        {
            clock_gettime(CLOCK_MONOTONIC, &rt_start_time);
        }

        MPI_Request send_request;
        MPI_Isend(message, message_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &send_request);
        MPI_Wait(&send_request, MPI_STATUS_IGNORE);

        // Print message information
        if (print_enabled)
        {
            timespec end_time;
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            timespec elapsed_time = diff(rt_start_time, end_time);

            double rtt = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);
            double throughput = (message_size * sizeof(int8_t) * 8) / (rtt * 1000 * 1000);

            continuous_send ? print_continuous(round_trip, rtt, throughput) : print_discrete(message_size, throughput);
        }
    }
    else if (rank == 1)
    {
        MPI_Request recv_request;
        MPI_Irecv(message, message_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &recv_request);
        MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    }
}

// Function to perform continuous round-trip communication
void setup_continuous_communication(int8_t *message, std::size_t message_size, int8_t rank, std::size_t print_interval)
{
    if (rank == 0)
        std::cout << "Message size: " << message_size << ", interval: " << print_interval << std::endl;

    // time of run start
    timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    std::size_t round_trip = 1;
    bool running = true;

    while (running)
    {
        // handle overflow
        if (round_trip == 0)
            clock_gettime(CLOCK_MONOTONIC, &start_time);

        perform_rt_communication(message, message_size, round_trip, rank, (round_trip % print_interval) == 0);
        round_trip++;

        // TODO: install ncurses on target?
        /*  ch = getch();
            if (tolower(ch))
                running = false; */

    }

    // time of run finish
    timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    timespec elapsed_time = diff(start_time, end_time);
    double elapsed_seconds = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);

    if (rank == 0)
        std::cout << "Elapsed time: " << elapsed_seconds << " s" << std::endl;
}

// Function to perform discrete round-trip communication
void setup_discrete_communication(int8_t *message, int8_t rank, std::size_t max_power = 24)
{
    if (rank == 0)
    {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "| " << std::left << std::setw(12) << "Bytes"
                    << " | " << std::setw(18) << "Throughput [Mbit/s]"
                    << " |\n";
        std::cout << "--------------------------------------\n";
    }

    std::size_t round_trip = 1;

    for (std::size_t power = 0; power <= max_power; power++)
    {
        std::size_t current_message_size = static_cast<std::size_t>(std::pow(2, power));
        perform_rt_communication(message, current_message_size, round_trip, rank, true, false);
        round_trip++;
    }
}

int main(int argc, char **argv)
{
    int rank, size;

    std::size_t message_size = 1000000; // message size in bytes
    std::size_t print_interval = 10000; // communication steps to be printed
    bool continuous_send = true;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Hello from " << rank << std::endl;

    if (size != 2)
    {
        std::cerr << "This benchmark requires exactly 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    // Process command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "m:i:sh")) != -1)
    {
        switch (opt)
        {
        case 'm':
            message_size = std::stoi(optarg);
            break;
        case 'i':
            print_interval = std::stoi(optarg);
            break;
        case 's':
            continuous_send = false;
            break;
        case 'h':
        default:
            if (rank == 0)
                std::cout << "Usage:" << argv[0] << std::endl
                          << "\t-m <message-size>\n\t-i <print-interval>\n\t-s perform scan\n\t-h <help>" << std::endl;
            MPI_Finalize();
            return 1;
        }
    }

    // Allocate memory with desired alignment
    long page_size = sysconf(_SC_PAGESIZE);
    void *mem = nullptr;
    if (posix_memalign(&mem, page_size, message_size) != 0)
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        return 1;
    }

    // Initialise message and data
    int8_t *message = static_cast<int8_t *>(mem);
    std::fill(message, message + message_size, 0);

    // Warmup round
    perform_rt_communication(message, message_size, 0, rank, false);

    // Scan operation on message sizes
    std::size_t total_rounds;

     // Initialize ncurses
    /*     initscr();
        cbreak();              // Disable line buffering
        noecho();              // Disable automatic echoing of characters
        nodelay(stdscr, true); // Make getch() non-blocking
        char ch; */


    if (continuous_send)
    {
        setup_continuous_communication(message, message_size, rank, print_interval);        
    }
    else
    {
        setup_discrete_communication(message, rank);
    }

    // Finalise program
    std::free(mem);
    // endwin();
    MPI_Finalize();

    return 0;
}

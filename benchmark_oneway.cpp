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
void print_continuous(double rtt, double throughput)
{
    std::cout << std::fixed << std::setprecision(8);

    std::cout << std::right << std::setw(14) << " Avg. RTT"
              << " | " << std::setw(25) << "Throughput"
              << std::endl;

    std::cout << std::right << std::setw(12) << rtt << " s"
              << " | " << std::setw(18) << std::fixed << std::setprecision(2) << throughput << " Mbit/s\n"
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
void perform_rt_communication(int8_t *message, std::size_t message_size, int8_t rank)
{
    if (rank == 0)
    {
        MPI_Request send_request;
        MPI_Isend(message, message_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &send_request);
        MPI_Wait(&send_request, MPI_STATUS_IGNORE);
    }
    else if (rank == 1)
    {
        MPI_Request recv_request;
        MPI_Irecv(message, message_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &recv_request);
        MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    }
}

void setup_continuous_communication(int8_t *message, std::size_t message_size, int8_t rank, std::size_t print_interval)
{
    if (rank == 0)
        std::cout << "Message size: " << message_size << ", interval: " << print_interval << std::endl;

    std::size_t message_count = 0;
    bool running = true;

    timespec end_time;
    timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (running)
    {
        perform_rt_communication(message, message_size, rank);
        message_count++;

        if (rank == 0 && (message_count % print_interval) == 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            timespec elapsed_time = diff(start_time, end_time);
            double elapsed_secs = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);

            double avg_rtt = elapsed_secs / print_interval;
            double avg_throughput = (print_interval * message_size * 8.0) / (elapsed_secs * 1e6);

            print_continuous(avg_rtt, avg_throughput);

            // reset counters
            message_count = 0;
            clock_gettime(CLOCK_MONOTONIC, &start_time);
        }

        // TODO: install ncurses on target?
        /*  ch = getch();
            if (tolower(ch))
                running = false; */
    }
}

void setup_discrete_communication(int8_t *message, int8_t rank, std::size_t max_power = 20)
{
    if (rank == 0)
    {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "| " << std::left << std::setw(12) << "Bytes"
                  << " | " << std::setw(18) << "Throughput [Mbit/s]"
                  << " |\n";
        std::cout << "--------------------------------------\n";
    }

    timespec end_time;
    timespec start_time;

    for (std::size_t power = 0; power <= max_power; power++)
    {
        std::size_t current_message_size = static_cast<std::size_t>(std::pow(2, power));
        std::size_t iteration_count = 10000;

        clock_gettime(CLOCK_MONOTONIC, &start_time);

        // Perform iteration_count sends for message of current_message_size
        for (std::size_t iteration = 0; iteration < iteration_count; iteration++)
            perform_rt_communication(message, current_message_size, rank);

        // Calculate and print average throughput for this message size
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        timespec elapsed_time = diff(start_time, end_time);
        double elapsed_secs = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);

        double avg_throughput = (iteration_count * current_message_size * 8.0) / (elapsed_secs * 1e6);

        if (rank == 0)
            print_discrete(current_message_size, avg_throughput);

        // Reset time
        clock_gettime(CLOCK_MONOTONIC, &start_time);
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

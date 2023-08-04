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
#include <csignal>
#include <vector>
#include <cstring>

// FIXME: sigint capture not making error but not working

/*
send smaller chunks
1.1. array is multiple of what I’m sending
1.2. the chunks should be sent from and recieved from circular buffer           // FIXME: circular buffer
1.3. warmup: send bigger chunks, make them overlap
*/

volatile sig_atomic_t sigintReceived = 0; // indicate if SIGINT has been received
timespec run_start_time;                  // time of run start once program is set up

// Signal handler function for SIGINT
void sigintHandler(int signal)
{
    sigintReceived = 1;
}

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

std::vector<std::pair<int, int>> find_subarray_indices(int message_size)
{
    int subarray_size = std::ceil(0.32 * message_size);
    int shared_elements = std::ceil(0.15 * message_size);

    std::vector<std::pair<int, int>> subarray_indices;

    for (int i = 0; i < 5; ++i)
    {
        int start = i * (subarray_size - shared_elements);
        int end = start + subarray_size;
        subarray_indices.push_back(std::make_pair(start, end));
    }

    return subarray_indices;
}

void print_elapsed()
{
    timespec run_end_time;
    clock_gettime(CLOCK_MONOTONIC, &run_end_time);
    timespec elapsed_time = diff(run_start_time, run_end_time);
    double elapsed_seconds = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);

    std::cout << "Total elapsed time: " << elapsed_seconds << " s" << std::endl;
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

void perform_warmup(int8_t *message, std::vector<std::pair<int, int>> subarray_indices, int8_t rank)
{
    std::size_t subarray_count = subarray_indices.size();
    std::size_t subarray_size;

    if (rank == 0)
        std::cout << "\nPerforming warmup...";

    if (rank == 0)
    {
        for (int i = 0; i < subarray_count; i++)
        {
            subarray_size = subarray_indices[i].second - subarray_indices[i].first + 1;
            MPI_Send(&message[subarray_indices[i].first], subarray_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        }
    }
    else if (rank == 1)
    {
        for (int i = 0; i < subarray_count; i++)
        {
            subarray_size = subarray_indices[i].second - subarray_indices[i].first + 1;
            MPI_Recv(&message[subarray_indices[i].first], subarray_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    if (rank == 0)
        std::cout << "Done.\n"
                  << std::endl;
}

// Function to perform round-trip communication between two processes
void perform_rt_communication(int8_t *message, std::size_t message_size, std::size_t chunk_size, std::size_t chunk_count,
                              int8_t rank, std::size_t print_interval)
{
    std::vector<MPI_Request> requests(chunk_count);
    std::vector<MPI_Status> statuses(chunk_count);

    const std::size_t buffer_size = 10; // Circular buffer size
    int8_t *buffer = new int8_t[buffer_size * chunk_size];
    int buffer_index = 0;

    if (rank == 0)
    {
        for (std::size_t i = 0; i < print_interval; i++)
        {
            for (std::size_t chunk = 0; chunk < chunk_count; chunk++)
            {
                std::size_t offset = chunk * chunk_size;
                MPI_Isend(&message[offset], chunk_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &requests[chunk]);
            }
            MPI_Waitall(chunk_count, requests.data(), statuses.data());

            // Store data in the circular buffer
            std::size_t buffer_offset = buffer_index * chunk_size;
            for (std::size_t chunk = 0; chunk < chunk_count; chunk++)
            {
                std::size_t message_offset = chunk * chunk_size;
                std::memcpy(&buffer[buffer_offset + message_offset], &message[message_offset], chunk_size);
            }

            buffer_index = (buffer_index + 1) % buffer_size;
        }
    }
    else if (rank == 1)
    {
        for (std::size_t i = 0; i < print_interval; i++)
        {
            // Receive data from the circular buffer
            std::size_t buffer_offset = buffer_index * chunk_size;
            for (std::size_t chunk = 0; chunk < chunk_count; chunk++)
            {
                std::size_t message_offset = chunk * chunk_size;
                MPI_Irecv(&message[message_offset], chunk_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &requests[chunk]);
            }
            MPI_Waitall(chunk_count, requests.data(), statuses.data());

            // Use data from the buffer
            for (std::size_t chunk = 0; chunk < chunk_count; chunk++)
            {
                std::size_t message_offset = chunk * chunk_size;
                std::memcpy(&message[message_offset], &buffer[buffer_offset + message_offset], chunk_size);
            }

            buffer_index = (buffer_index + 1) % buffer_size;
        }
    }

    delete[] buffer;
}

void setup_continuous_communication(int8_t *message, std::size_t message_size, std::size_t chunk_size,
                                    int8_t rank, std::size_t print_interval)
{
    if (rank == 0)
        std::cout << "Message size [B]: " << message_size << std::endl
                  << "Chunk size [B]: " << chunk_size << std::endl
                  << "Interval: " << print_interval << "\n"
                  << std::endl;

    std::size_t message_count = 0;
    bool running = true;

    std::size_t chunk_count = static_cast<std::size_t>(
        std::ceil(
            static_cast<double>(message_size) / chunk_size));

    timespec end_time;
    timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (running)
    {
        perform_rt_communication(message, message_size, chunk_size, chunk_count, rank, print_interval);

        clock_gettime(CLOCK_MONOTONIC, &end_time);
        timespec elapsed_time = diff(start_time, end_time);
        double elapsed_secs = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);

        double avg_rtt = elapsed_secs / print_interval;
        double avg_throughput = (print_interval * message_size * 8.0) / (elapsed_secs * 1e6);

        if (rank == 0)
            print_continuous(avg_rtt, avg_throughput);

        // Reset counters
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        if (sigintReceived)
        {
            if (rank == 0)
                print_elapsed();
            std::exit(EXIT_SUCCESS);
        }
    }
}

void setup_discrete_communication(int8_t *message, std::size_t chunk_size, int8_t rank, std::size_t max_power = 22)
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

        std::size_t chunk_count = static_cast<std::size_t>(
            std::ceil(
                static_cast<double>(current_message_size) / chunk_size));

        clock_gettime(CLOCK_MONOTONIC, &start_time);

        // Perform iteration_count sends for message of current_message_size
        for (std::size_t iteration = 0; iteration < iteration_count; iteration++)
            perform_rt_communication(message, current_message_size, chunk_size, chunk_count, rank, iteration_count);

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

    std::size_t message_size = 10e6;   // message size in bytes
    std::size_t print_interval = 10e4; // communication steps to be printed
    std::size_t chunk_size = 10e5;     // chunk size in bytes
    bool continuous_send = true;

    std::size_t min_message_size = 10e4; // ?
    std::size_t min_interval = 10e5;
    std::size_t max_power = 22;

    std::signal(SIGINT, sigintHandler);

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
    std::size_t tmp;
    while ((opt = getopt(argc, argv, "m:i:c:sh")) != -1)
    {
        switch (opt)
        {
        case 's':
            continuous_send = false;
            break;
        case 'm':
            if (!continuous_send)
                break;
            tmp = std::stoi(optarg);
            message_size = (tmp >= min_message_size) ? tmp : min_message_size;
            break;
        case 'i':
            if (!continuous_send)
                break;
            tmp = std::stoi(optarg);
            print_interval = (tmp >= min_interval) ? tmp : min_interval;
            break;
        case 'c':
            if (!continuous_send)
                break;
            chunk_size = std::stoi(optarg); //(tmp >= min_chunk_size) ? tmp : min_chunk_size;
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

    // Validate chunk size
    if (chunk_size > message_size)
    {
        chunk_size = message_size;
        std::cout << "Chunk size cannot be greater than message size. Defaulting to no chunks." << std::endl;
    }

    // Allocate memory with desired alignment
    long page_size = sysconf(_SC_PAGESIZE);
    void *mem = nullptr;
    std::size_t align_size = continuous_send ? message_size : static_cast<std::size_t>(std::pow(2, max_power));

    if (posix_memalign(&mem, page_size, align_size) != 0)
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        return 1;
    }

    // Initialise message and data
    int8_t *message = static_cast<int8_t *>(mem);
    std::fill(message, message + align_size, 0);

    std::unique_ptr<void, decltype(&free)> mem_ptr(mem, &free);

    std::vector<std::pair<int, int>> subarray_indices = find_subarray_indices(message_size);

    // Perform warmup
    perform_warmup(message, subarray_indices, rank);

    // Run program
    clock_gettime(CLOCK_MONOTONIC, &run_start_time);

    if (continuous_send)
    {
        setup_continuous_communication(message, message_size, chunk_size, rank, print_interval);
    }
    else
    {
        setup_discrete_communication(message, chunk_size, rank);
    }

    // Finalise program
    if (rank == 0)
        print_elapsed();
    MPI_Finalize();

    return 0;
}
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
#include <algorithm>

// FIXME: sigint capture not making error but not working
// TODO: check for error status handling needs


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
        time_diff.tv_nsec = 1e9 + end.tv_nsec - start.tv_nsec;
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

void parse_arguments(int argc, char **argv, int rank, std::size_t min_message_size, std::size_t min_interval,
                     bool &continuous_send, std::size_t &message_size, std::size_t &print_interval,
                     std::size_t &buffer_size, std::size_t &warmup_iterations)
{
    int opt;
    std::size_t tmp;
    while ((opt = getopt(argc, argv, "m:i:c:b:w:sh")) != -1)
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
        case 'b':
            tmp = std::stoi(optarg);
            if (tmp > 0)
                buffer_size = tmp;
            break;
        case 'w':
            tmp = std::stoi(optarg);
            if (tmp > 0)
                warmup_iterations = tmp;
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

void print_elapsed()
{
    timespec run_end_time;
    clock_gettime(CLOCK_MONOTONIC, &run_end_time);
    timespec elapsed_time = diff(run_start_time, run_end_time);
    double elapsed_seconds = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);

    std::cout << "Total elapsed time: " << elapsed_seconds << " s" << std::endl;
}

std::pair<double, double> calculate_throughput(timespec start_time, timespec end_time, std::size_t message_size, std::size_t interval)
{
    timespec elapsed_time = diff(start_time, end_time);
    double elapsed_secs = elapsed_time.tv_sec + (elapsed_time.tv_nsec / 1e9);

    double avg_rtt = elapsed_secs / interval;
    double avg_throughput = (interval * message_size * 8.0) / (elapsed_secs * 1e6);

    return std::make_pair(avg_rtt, avg_throughput);
}

// Functions for formatted throughput info printing
void print_continuous(double rtt, double throughput, int error_messages_count)
{
    std::cout << std::fixed << std::setprecision(8);

    std::cout << std::right << std::setw(14) << " Avg. RTT"
              << " | " << std::setw(25) << "Throughput"
              << " | " << std::setw(10) << " Errors"
              << std::endl;

    std::cout << std::right << std::setw(12) << rtt << " s"
              << " | " << std::setw(18) << std::fixed << std::setprecision(2) << throughput << " Mbit/s"
              << " | " << std::setw(10) << error_messages_count << std::endl
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
        std::cout << "Performing warmup...";

    if (rank == 0)
    {
        for (int i = 0; i < subarray_count; i++)
        {
            subarray_size = subarray_indices[i].second - subarray_indices[i].first + 1;
            MPI_Send(message + subarray_indices[i].first, subarray_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        }
    }
    else if (rank == 1)
    {
        for (int i = 0; i < subarray_count; i++)
        {
            subarray_size = subarray_indices[i].second - subarray_indices[i].first + 1;
            MPI_Recv(message + subarray_indices[i].first, subarray_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    if (rank == 0)
        std::cout << "Done." << std::endl;
}

// Function to perform round-trip communication between two processes
int perform_rt_communication(std::vector<int8_t> &snd_buffer, std::vector<int8_t> &rcv_buffer, std::size_t buffer_size,
                             int8_t *message, std::size_t message_size,
                             int8_t rank, std::size_t interval)
{
    std::vector<MPI_Status> statuses(interval);

    std::size_t buffer_index = 0;

    if (rank == 0)
    {
        for (std::size_t i = 0; i < interval; i++)
        {
            // TODO: send from buffer, not message
            MPI_Send(message, message_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            buffer_index = (buffer_index + 1) % buffer_size;
        }
    }

    else if (rank == 1)
    {
        for (std::size_t i = 0; i < interval; i++)
        {
            MPI_Recv(message, message_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &statuses[i]);
            buffer_index = (buffer_index + 1) % buffer_size;
        }
    }

    return std::count_if(statuses.begin(), statuses.end(),
                         [](const MPI_Status &status)
                         { return status.MPI_ERROR != MPI_SUCCESS; });
}

void setup_continuous_communication(std::vector<int8_t> &snd_buffer, std::vector<int8_t> &rcv_buffer, std::size_t buffer_size,
                                    int8_t *message, std::size_t message_size,
                                    int8_t rank, std::size_t print_interval)
{
    if (rank == 0)
        std::cout << "Message size [B]: " << message_size << std::endl
                  << "Interval: " << print_interval << "\n"
                  << std::endl;

    double avg_rtt, avg_throughput;
    int error_messages_count;
    std::size_t message_count = 0;
    bool running = true;

    timespec end_time;
    timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (running)
    {
        error_messages_count = perform_rt_communication(snd_buffer, rcv_buffer, buffer_size, message, message_size, rank, print_interval);

        clock_gettime(CLOCK_MONOTONIC, &end_time);
        std::tie(avg_rtt, avg_throughput) = calculate_throughput(start_time, end_time, message_size, print_interval);

        if (rank == 0)
            print_continuous(avg_rtt, avg_throughput, error_messages_count);

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

void setup_discrete_communication(std::vector<int8_t> &snd_buffer, std::vector<int8_t> &rcv_buffer, std::size_t buffer_size, int8_t *message,
                                  int8_t rank, std::size_t max_power = 22)
{
    if (rank == 0)
    {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "| " << std::left << std::setw(12) << "Bytes"
                  << " | " << std::setw(18) << "Throughput [Mbit/s]"
                  << " |\n";
        std::cout << "--------------------------------------\n";
    }

    double avg_throughput;
    int error_messages_total = 0;
    timespec end_time;
    timespec start_time;

    for (std::size_t power = 0; power <= max_power; power++)
    {
        std::size_t current_message_size = static_cast<std::size_t>(std::pow(2, power));
        std::size_t iteration_count = 10000;

        clock_gettime(CLOCK_MONOTONIC, &start_time);

        // Perform iteration_count sends for message of current_message_size
        error_messages_total += perform_rt_communication(snd_buffer, rcv_buffer, buffer_size, message, current_message_size, rank, iteration_count);

        // Calculate and print average throughput for this message size
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        std::tie(std::ignore, avg_throughput) = calculate_throughput(start_time, end_time, current_message_size, iteration_count);

        if (rank == 0)
            print_discrete(current_message_size, avg_throughput);

        // Reset time
        clock_gettime(CLOCK_MONOTONIC, &start_time);
    }

    if (rank == 0)
        std::cout << "Number of non-MPI_SUCCESS statuses: " << error_messages_total << "\n"
                  << std::endl;
}

int main(int argc, char **argv)
{
    int rank, size;

    std::size_t message_size = 1e6;      // message size in bytes
    std::size_t print_interval = 1e4;    // communication steps to be printed
    std::size_t buffer_size = 10;        // circular buffer size (in chunk count)
    std::size_t warmup_iterations = 100; // iteration count for warmup-related throughput calculation
    bool continuous_send = true;

    std::size_t min_message_size = 1e4;
    std::size_t min_interval = 1e5;
    std::size_t max_power = 22;

    std::signal(SIGINT, sigintHandler);

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

    parse_arguments(argc, argv, rank, min_message_size, min_interval,
                    continuous_send, message_size, print_interval, buffer_size, warmup_iterations);

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
    // TODO: replace with variable size message/buffer initialisation
    int8_t *message = static_cast<int8_t *>(mem);
    std::fill(message, message + align_size, 0);

    MPI_Bcast(&mem, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> mem_ptr(rank == 0 ? mem : nullptr, &free);

    std::vector<int8_t> snd_buffer(buffer_size * message_size);
    std::vector<int8_t> rcv_buffer(buffer_size * message_size);

    // Perform warmup
    std::vector<std::pair<int, int>> subarray_indices = find_subarray_indices(message_size);
    timespec warmup_start_time, warmup_end_time;
    double warmup_throughput;

    clock_gettime(CLOCK_MONOTONIC, &warmup_start_time);
    perform_rt_communication(snd_buffer, rcv_buffer, buffer_size, message, message_size, rank, warmup_iterations);
    clock_gettime(CLOCK_MONOTONIC, &warmup_end_time);
    std::tie(std::ignore, warmup_throughput) = calculate_throughput(warmup_start_time, warmup_end_time, message_size, warmup_iterations);
    if (rank == 0)
        std::cout << "\nPre-warmup throughput: " << warmup_throughput << " Mbit/s" << std::endl;

    perform_warmup(message, subarray_indices, rank);

    clock_gettime(CLOCK_MONOTONIC, &warmup_start_time);
    perform_rt_communication(snd_buffer, rcv_buffer, buffer_size, message, message_size, rank, warmup_iterations);
    clock_gettime(CLOCK_MONOTONIC, &warmup_end_time);
    std::tie(std::ignore, warmup_throughput) = calculate_throughput(warmup_start_time, warmup_end_time, message_size, warmup_iterations);
    if (rank == 0)
        std::cout << "Post-warmup throughput: " << warmup_throughput << " Mbit/s\n"
                  << std::endl;

    // Run program
    clock_gettime(CLOCK_MONOTONIC, &run_start_time);

    if (continuous_send)
    {
        setup_continuous_communication(snd_buffer, rcv_buffer, buffer_size, message, message_size, rank, print_interval);
    }
    else
    {
        setup_discrete_communication(snd_buffer, rcv_buffer, buffer_size, message, rank);
    }

    // Finalise program
    if (rank == 0)
        print_elapsed();
    MPI_Finalize();

    return 0;
}
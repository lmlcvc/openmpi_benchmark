#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <mpi.h>
#include <chrono>

void perform_rt_communication(uint32_t *message, std::size_t message_size, std::size_t round_trip,
                              int8_t rank, bool print_enabled = false)
{

    if (rank == 0)
    {
        double start_time = print_enabled ? MPI_Wtime() : 0.0;

        MPI_Request send_request;
        MPI_Isend(message, message_size * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD,
                  &send_request);
        MPI_Wait(&send_request, MPI_STATUS_IGNORE);

        // Print message information
        if (print_enabled)
        {
            double rtt = MPI_Wtime() - start_time;
            double rtt_throughput = (message_size * sizeof(uint32_t) * 8) / (rtt * 1000 * 1000);

            std::cout << "Round Trip " << round_trip
                      << " - RTT: " << rtt
                      << " s | Throughput: " << rtt_throughput << " Mbit/s"
                      << std::endl;
        }
    }
    else if (rank == 1)
    {
        MPI_Request recv_request;
        MPI_Irecv(message, message_size * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD,
                  &recv_request);
        MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char **argv)
{
    int rank, size;
    uint32_t *message;
    double start_time, end_time, elapsed_time;

    std::size_t message_size = 1000000;
    std::size_t print_interval = 1000; // communication steps to be printed

    bool running = true;

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
    while ((opt = getopt(argc, argv, "m:i:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            message_size = std::stoi(optarg);
            break;
        case 'i':
            print_interval = std::stoi(optarg);
            break;
        default:
            std::cerr << "Usage: " << argv[0]
                      << " -m <message-size> -i <print-interval>" << std::endl;
            return 1;
        }
    }

    std::cout << "Message size: " << message_size << ", interval: " << print_interval << std::endl;

    // Initialize message and data
    message = new uint32_t[message_size];
    for (std::size_t i = 0; i < message_size; i++)
    {
        message[i] = i;
    }

    // Warmup round
    perform_rt_communication(message, message_size, 0, rank, false);

    // Continuous communication run
    std::size_t round_trip = 1;
    start_time = MPI_Wtime();
    double current_time;

    while (running)
    {
        // handle overflow
        if (round_trip == 0)
            start_time = MPI_Wtime();

        perform_rt_communication(message, message_size, round_trip, rank, (round_trip % print_interval) == 0);
        round_trip++;

        // exit on q
        // FIXME if (kbhit())  running = false;
    }

    end_time = MPI_Wtime();
    elapsed_time = end_time - start_time;

    if (rank == 0)
        std::cout << "Elapsed time: " << elapsed_time << " s" << std::endl;

    // Free allocated memory and finalize
    delete[] message;
    MPI_Finalize();

    return 0;
}

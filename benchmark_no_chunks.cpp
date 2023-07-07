#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <mpi.h>
#include <chrono>


void perform_rt_communication(uint32_t *message, std::size_t message_size, std::size_t round_trip,
                              int8_t rank, bool print_enabled = true) {
    double start_time = MPI_Wtime();

    MPI_Request send_request, recv_request;
    MPI_Isend(&message, message_size * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD,
              &send_request);
    MPI_Irecv(&message, message_size * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD,
              &recv_request);

    // Wait for communication to complete
    MPI_Wait(&send_request, MPI_STATUS_IGNORE);
    MPI_Wait(&recv_request, MPI_STATUS_IGNORE);

    // Print message information
    if (rank == 0 && print_enabled) {
        double rtt = MPI_Wtime() - start_time;
        double rtt_throughput = (2.0 * message_size * sizeof(uint32_t)) / (rtt * 1000 * 1000 * 8);

        std::cout << "Round Trip " << round_trip
                << " - RTT: " << rtt
                << " s | Throughput: " << rtt_throughput << " Mbit/s"
                << std::endl;
    }

}


int main(int argc, char **argv) {
    int rank, size;
    uint32_t *message;
    double start_time, end_time, elapsed_time;

    int opt;
    std::size_t message_size = 1000000;
    double interval_ms = 100.0;

    bool running = true;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Hello from " << rank << std::endl;

    if (size != 2) {
        std::cerr << "This benchmark requires exactly 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    // Process command line arguments
    while ((opt = getopt(argc, argv, "m:i:")) != -1) {
        switch (opt) {
            case 'm':
                message_size = std::stoi(optarg);
                break;
            case 'i':
                interval_ms = std::stoi(optarg);
                break;
            default:
                std::cerr << "Usage: " << argv[0]
                          << " -m <message-size> -i <interval-ms>" << std::endl;
                return 1;
        }
    }

    // Initialize message and data
    message = new uint32_t[message_size];
    for (std::size_t i = 0; i < message_size; i++) {
        message[i] = i;
    }

    // Warmup round
    perform_rt_communication(message, message_size, -1, rank);

    // Continuous communication run
    std::size_t round_trip = 1;
    start_time = MPI_Wtime();
    double current_time;

    while (running) {
        // handle overflow
        if (round_trip == 0) start_time = MPI_Wtime();
        current_time = MPI_Wtime();
        elapsed_time = current_time - start_time;

        // perform RT communication and measurements
        if (elapsed_time * 1000 >= round_trip * interval_ms) {
            perform_rt_communication(message, message_size, round_trip, rank);
        }

        // exit on q
        // FIXME if (kbhit())  running = false;

        round_trip++;
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

#include <iostream>
#include <iomanip>
#include <mpi.h>
#include <chrono>
#include <cstring>

// TODO run old code w/o chunks across 2 nodes

// TODO https://www.man7.org/linux/man-pages/man3/getopt.3.html


std::size_t message_size = 1000000;
std::size_t chunk_size = 1000;
std::size_t num_round_trips = 10;
double interval_ms = 50.0;

void print_chunk_info(uint8_t rank, std::size_t chunk, double rtt, double chunk_throughput) {
    if (rank == 0) {
        std::cout << "Chunk " << std::setw(2) << chunk
                  << " | Time: " << std::fixed << std::setprecision(8) << rtt
                  << " ms | Throughput: " << std::fixed << std::setprecision(2) << chunk_throughput
                  << " Mbit/s" << std::endl;
    }
}

void perform_rt_communication(uint32_t *message, uint8_t rank, bool print_enabled = true) {
    std::size_t num_chunks = message_size / chunk_size;
    std::size_t remaining_elements = message_size % chunk_size;

    for (std::size_t chunk = 0; chunk < num_chunks; chunk++) {
        std::size_t offset = chunk * chunk_size;
        double start_time = MPI_Wtime();

        MPI_Request send_request, recv_request;
        MPI_Isend(&message[offset], chunk_size * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD, &send_request);
        MPI_Irecv(&message[offset], chunk_size * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD, &recv_request);

        // Wait for communication to complete
        MPI_Wait(&send_request, MPI_STATUS_IGNORE);
        MPI_Wait(&recv_request, MPI_STATUS_IGNORE);

        // Print chunk information (if needed)
        if (rank == 0 && print_enabled) {
            double end_time = MPI_Wtime();
            double rtt = end_time - start_time;
            double chunk_throughput = (2.0 * chunk_size * sizeof(uint32_t)) / (rtt * 1000 * 1000 * 8);

            print_chunk_info(rank, chunk, rtt, chunk_throughput);
        }
    }

    std::size_t offset = num_chunks * chunk_size;

    if (remaining_elements) {
        double start_time = MPI_Wtime();

        MPI_Request send_request, recv_request;
        MPI_Isend(&message[offset], remaining_elements * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD, &send_request);
        MPI_Irecv(&message[offset], remaining_elements * sizeof(uint32_t), MPI_BYTE, 1 - rank, 0, MPI_COMM_WORLD, &recv_request);

        MPI_Wait(&send_request, MPI_STATUS_IGNORE);
        MPI_Wait(&recv_request, MPI_STATUS_IGNORE);

        if (rank == 0 && print_enabled) {
            double end_time = MPI_Wtime();
            double rtt = end_time - start_time;
            double chunk_throughput = (2.0 * remaining_elements * sizeof(uint32_t)) / (rtt * 1000 * 1000 * 8);

            print_chunk_info(rank, num_chunks + 1, rtt, chunk_throughput);
        }
    }
}

int main(int argc, char **argv) {
    int rank;
    int size;
    uint32_t *message;
    double start_time, elapsed_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Hello from " << rank << std::endl;

    if (size != 2) {
        std::cerr << "This benchmark requires exactly 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--message-size") == 0 && i + 1 < argc) {
            message_size = std::stoul(argv[i + 1]);
        } else if (std::strcmp(argv[i], "--chunk-size") == 0 && i + 1 < argc) {
            chunk_size = std::stoul(argv[i + 1]);
        } else if (std::strcmp(argv[i], "--num-round-trips") == 0 && i + 1 < argc) {
            num_round_trips = std::stoul(argv[i + 1]);
        } else if (std::strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            interval_ms = std::stod(argv[i + 1]);
        }
    }

    // Initialize message and data
    message = new uint32_t[message_size];
    for (std::size_t i = 0; i < message_size; i++) {
        message[i] = i;
    }

    // Warmup round
    perform_rt_communication(message, rank, false);

    // Measure elapsed time
    start_time = MPI_Wtime();
    double current_time;
    double elapsed_ms = 0.0;
    std::size_t round_trip = 0;

    while (round_trip < num_round_trips) {
        current_time = MPI_Wtime();
        elapsed_time = current_time - start_time;
        elapsed_ms = elapsed_time * 1000.0;

        if (elapsed_ms >= (round_trip + 1) * interval_ms) {
            double rt_start_time = MPI_Wtime();
            perform_rt_communication(message, rank);
            double rt_end_time = MPI_Wtime();
            double rtt = rt_end_time - rt_start_time;

            if (rank == 0) {
                std::cout << "Round Trip " << round_trip + 1
                        << " | Total RTT: " << std::fixed << std::setprecision(8) << rtt
                        << " ms " << std::endl << "\t---------------------" << std::endl;

                round_trip++;
            }
        }
    }

    // Free allocated memory and finalize
    delete[] message;
    MPI_Finalize();

    return 0;
}
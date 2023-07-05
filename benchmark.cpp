#include <iostream>
#include <iomanip>
#include <mpi.h>
#include <chrono>
#include <cstring>

std::size_t message_size = 1000000;
std::size_t chunk_size = 1000;
std::size_t num_round_trips = 10;
double interval_ms = 50.0;

void perform_rt_communication(uint32_t* message, uint8_t rank) {
    MPI_Datatype mpi_uint32_t;
    MPI_Type_contiguous(sizeof(uint32_t), MPI_BYTE, &mpi_uint32_t);
    MPI_Type_commit(&mpi_uint32_t);

    std::size_t num_chunks = message_size / chunk_size;
    std::size_t remaining_elements = message_size % chunk_size;

    for (std::size_t chunk = 0; chunk < num_chunks; chunk++) {
        std::size_t offset = chunk * chunk_size;
        double start_time = MPI_Wtime();

        MPI_Send(&message[offset], chunk_size, mpi_uint32_t, 1 - rank, 0, MPI_COMM_WORLD);
        MPI_Recv(&message[offset], chunk_size, mpi_uint32_t, 1 - rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        double end_time = MPI_Wtime();

        double rtt = end_time - start_time;
        double rtt_throughput = static_cast<double>(chunk_size * sizeof(uint32_t)) / (rtt * 1024 * 1024 * 8);

        if (rank == 0) {
            std::cout << "Chunk " << chunk + 1
                      << " | RTT: " << std::fixed << std::setprecision(6) << rtt
                      << " ms | Throughput: " << std::fixed << std::setprecision(2) << rtt_throughput
                      << " Mbps" << std::endl;
        }
    }

    std::size_t offset = num_chunks * chunk_size;

    // Pad remaining elements with 0s
    if (remaining_elements > 0) {
        std::fill(&message[offset], &message[offset + remaining_elements], 0);
    }

    double start_time = MPI_Wtime();
    MPI_Send(&message[offset], chunk_size, mpi_uint32_t, 1 - rank, 0, MPI_COMM_WORLD);
    MPI_Recv(&message[offset], chunk_size, mpi_uint32_t, 1 - rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    double end_time = MPI_Wtime();

    double rtt = end_time - start_time;
    double rtt_throughput = static_cast<double>(chunk_size * sizeof(uint32_t)) / (rtt * 1024 * 1024 * 8);

    if (rank == 0) {
        std::cout << "Chunk " << num_chunks + 1
                  << " | RTT: " << std::fixed << std::setprecision(6) << rtt
                  << " ms | Throughput: " << std::fixed << std::setprecision(2) << rtt_throughput
                  << " Mbps" << std::endl;
    }

    MPI_Type_free(&mpi_uint32_t);
}

int main(int argc, char** argv) {
    int rank;
    int size;
    uint32_t* message;
    double start_time, end_time, elapsed_time;

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
    perform_rt_communication(message, rank);

    // Measure elapsed time
    start_time = MPI_Wtime();
    double elapsed_ms = 0.0;
    std::size_t round_trip = 0;

    std::size_t total_bytes = 0;
    double total_rtt = 0.0;

    while (elapsed_ms < num_round_trips * interval_ms) {
        double current_time = MPI_Wtime();
        elapsed_time = current_time - start_time;
        elapsed_ms = elapsed_time * 1000.0;

        if (elapsed_ms >= (round_trip + 1) * interval_ms) {
            double rt_start_time = MPI_Wtime();
            perform_rt_communication(message, rank);
            double rt_end_time = MPI_Wtime();
            double rtt = rt_end_time - rt_start_time;

            total_bytes += message_size * sizeof(uint32_t);
            total_rtt += rtt;

            if (rank == 0) {
                double rtt_throughput = static_cast<double>(total_bytes) / (total_rtt * 1024 * 1024 * 8);
                double avg_throughput = (rtt_throughput * (round_trip + 1)) / (round_trip + 2);

                std::cout << "Round Trip " << round_trip + 1
                          << " | RTT: " << std::fixed << std::setprecision(6) << rtt
                          << " ms | Throughput: " << std::fixed << std::setprecision(2) << rtt_throughput
                          << " Mbps | Average Throughput: " << std::fixed << std::setprecision(2) << avg_throughput
                          << " Mbps" << std::endl;

                round_trip++;
            }
        }
    }

    end_time = MPI_Wtime();
    elapsed_time = end_time - start_time;

    // Free allocated memory and finalize
    delete[] message;
    MPI_Finalize();

    return 0;
}

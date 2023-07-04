#include <iostream>
#include <iomanip>
#include <mpi.h>
#include <chrono>

const std::size_t MESSAGE_SIZE = 1000000;
const std::size_t NUM_ROUND_TRIPS = 10;
const double INTERVAL_MS = 50.0;

void perform_rt_communication(uint32_t *message, uint8_t rank) {
    MPI_Datatype mpi_uint32_t;
    MPI_Type_contiguous(sizeof(uint32_t), MPI_BYTE, &mpi_uint32_t);
    MPI_Type_commit(&mpi_uint32_t);

    if (rank == 0) {
        MPI_Send(message, MESSAGE_SIZE, mpi_uint32_t, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(message, MESSAGE_SIZE, mpi_uint32_t, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else if (rank == 1) {
        MPI_Recv(message, MESSAGE_SIZE, mpi_uint32_t, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(message, MESSAGE_SIZE, mpi_uint32_t, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Type_free(&mpi_uint32_t);
}

int main(int argc, char **argv) {
    int rank;
    int size;
    uint32_t *message;
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

    // Initialize message and data
    message = new uint32_t[MESSAGE_SIZE];
    for (std::size_t i = 0; i < MESSAGE_SIZE; i++) {
        message[i] = i;
    }
    std::cout << "Message Size: " << static_cast<double>(MESSAGE_SIZE * sizeof(uint32_t) / 1024) << " kB" << std::endl;

    // Warmup round
    perform_rt_communication(message, rank);

    // Measure elapsed time
    start_time = MPI_Wtime();
    double elapsed_ms = 0.0;
    std::size_t round_trip = 0;

    std::size_t total_bytes = 0;
    double total_rtt = 0.0;

    while (elapsed_ms < NUM_ROUND_TRIPS * INTERVAL_MS) {
        double current_time = MPI_Wtime();
        elapsed_time = current_time - start_time;
        elapsed_ms = elapsed_time * 1000.0;

        if (elapsed_ms >= (round_trip + 1) * INTERVAL_MS) {
            double rt_start_time = MPI_Wtime();
            perform_rt_communication(message, rank);
            double rt_end_time = MPI_Wtime();
            double rtt = rt_end_time - rt_start_time;

            total_bytes += MESSAGE_SIZE * sizeof(uint32_t);
            total_rtt += rtt;

            if (rank == 0) {
                double rtt_throughput = static_cast<double>(total_bytes) / (total_rtt * 1024 * 1024);
                double avg_throughput = (rtt_throughput * (round_trip + 1)) / (round_trip + 2);

                std::cout << "Round Trip " << round_trip + 1
                        << " | RTT: " << std::fixed << std::setprecision(6) << rtt
                        << " ms | Throughput: " << std::fixed << std::setprecision(2) << rtt_throughput
                        << " MB/s | Average Throughput: " << std::fixed << std::setprecision(2) << avg_throughput
                        << " MB/s" << std::endl;

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

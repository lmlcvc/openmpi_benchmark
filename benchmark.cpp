#include <iostream>
#include <mpi.h>
#include <chrono>

const std::size_t MESSAGE_SIZE = 1000000;
const std::size_t NUM_ROUND_TRIPS = 5;

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

    // Warmup round
    perform_rt_communication(message, rank);

    // Measure elapsed time
    start_time = MPI_Wtime();
    for (std::size_t i = 0; i < NUM_ROUND_TRIPS; i++) {
        perform_rt_communication(message, rank);
        if (rank == 0) {
            double rtt = MPI_Wtime() - start_time;
            double rtt_throughput = (double) (MESSAGE_SIZE * sizeof(uint32_t)) / (1024 * 1024 * rtt);

            std::cout << "Round Trip " << i + 1 << " - RTT: " << rtt << " s | Throughput: " << rtt_throughput << " MB/s"
                      << std::endl;
        }
    }
    end_time = MPI_Wtime();
    elapsed_time = end_time - start_time;

    if (rank == 0) {
        std::cout << "Elapsed time: " << elapsed_time << " s" << std::endl;
        double elapsed_throughput = (double) (MESSAGE_SIZE * sizeof(uint32_t)) / (1024 * 1024 * elapsed_time);
        std::cout << "Elapsed Throughput: " << elapsed_throughput << " MB/s" << std::endl;
    }

    // Free allocated memory and finalize
    delete[] message;
    MPI_Finalize();

    return 0;
}

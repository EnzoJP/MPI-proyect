/*
 * mpi_dynamic.cpp - MPI version with dynamic row distribution
 *
 * Compilation:
 *   mpic++ -o mpi_dynamic mpi_dynamic.cpp
 *
 * Usage:
 *   mpirun -np <P> ./mpi_dynamic <M> <R> [seed]
 *
 *   <M>    : number of rows    (positive integer)
 *   <R>    : number of columns (positive integer)
 *   [seed] : optional random seed shared across versions
 *
 * Example:
 *   mpirun -np 4 ./mpi_dynamic 1000 1000 12345
 */

#include <mpi.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

static const int MAX_VAL = 1000000000;
static const int MIN_VAL = 5000000;
static const int DYNAMIC_CHUNK_ROWS = 1;
static const int TAG_TASK = 1;
static const int TAG_RESULT = 2;

inline int generateValue(unsigned int seed, long long index)
{
    // Fast deterministic hash to generate value at specific index
    unsigned int x = seed ^ (index >> 32);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    x ^= index & 0xFFFFFFFF;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return MIN_VAL + (x % (MAX_VAL - MIN_VAL + 1));
}

struct Task {
    int startRow;
    int rowCount;
};

bool isPrime(int n)
{
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;

    int raiz = static_cast<int>(std::sqrt(static_cast<double>(n)));
    for (int i = 3; i <= raiz; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

long long countPrimesInRowRange(int startRow, int rowCount, int R, unsigned int seed)
{
    long long count = 0;

    // Generate and count primes row by row without storing matrix
    long long baseIndex = (long long)startRow * R;
    for (int i = 0; i < rowCount; ++i) {
        for (int j = 0; j < R; ++j) {
            int value = generateValue(seed, baseIndex + j);
            if (isPrime(value)) {
                ++count;
            }
        }
        baseIndex += R;
    }

    return count;
}

void sendTask(int destination, int startRow, int rowCount)
{
    int buf[2]; buf[0] = startRow; buf[1] = rowCount;
    MPI_Send(buf, 2, MPI_INT, destination, TAG_TASK, MPI_COMM_WORLD);
}

void sendChunk(int destination, const int* matrix, int startRow, int rowCount, int R)
{
    MPI_Send(matrix + (startRow * R), rowCount * R, MPI_INT, destination, TAG_TASK, MPI_COMM_WORLD);
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3 && argc != 4) {
        if (rank == 0) {
            std::cerr << "\n[ERROR] Incorrect number of arguments.\n"
                      << "  Correct usage : mpirun -np <P> " << argv[0] << " <M> <R> [seed]\n"
                      << "  Example       : mpirun -np 4 " << argv[0] << " 1000 1000 12345\n\n";
        }
        MPI_Finalize();
        return 1;
    }

    int M = std::atoi(argv[1]);
    int R = std::atoi(argv[2]);
    unsigned int seed = (argc == 4) ? static_cast<unsigned int>(std::strtoul(argv[3], nullptr, 10)) : 12345u;

    if (M <= 0 || R <= 0) {
        if (rank == 0) {
            std::cerr << "\n[ERROR] M and R must be positive integers.\n"
                      << "  Received: M=" << argv[1] << "  R=" << argv[2] << "\n\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        std::cout << "-------------------------------------------------\n"
                  << " MPI DYNAMIC VERSION - Prime count by rows\n"
                  << "-------------------------------------------------\n"
                  << " Rows    (M) : " << M << "\n"
                  << " Columns (R) : " << R << "\n"
                  << " Total elems  : " << static_cast<long long>(M) * R << "\n"
                  << " Value range  : [" << MIN_VAL << ", " << MAX_VAL << "]\n"
                  << " Processes    : " << size << "\n"
                  << " Seed         : " << seed << "\n"
                  << " Chunk rows   : " << DYNAMIC_CHUNK_ROWS << "\n"
                  << "-------------------------------------------------\n";
    }

    // Broadcast seed to all processes
    MPI_Bcast(&seed, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "[INFO] Matrix will be generated on-demand by workers\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double processStart = MPI_Wtime();

    // Per-process compute time (ms) for reductions/diagnostics
    double localTimeMs = 0.0;

    long long totalCount = 0;

    if (size == 1) {
        // Single process: generate and count all rows on-the-fly
        double t0 = MPI_Wtime();
        totalCount = countPrimesInRowRange(0, M, R, seed);
        double t1 = MPI_Wtime();
        localTimeMs = (t1 - t0) * 1000.0;
    } else if (rank == 0) {
        // Master: distribute tasks and collect results
        int nextRow = 0;
        int activeWorkers = 0;

        for (int worker = 1; worker < size; ++worker) {
            if (nextRow < M) {
                int rowsToSend = std::min(DYNAMIC_CHUNK_ROWS, M - nextRow);
                sendTask(worker, nextRow, rowsToSend);
                nextRow += rowsToSend;
                ++activeWorkers;
            } else {
                sendTask(worker, 0, 0);
            }
        }

        while (activeWorkers > 0) {
            long long partialCount = 0;
            MPI_Status status{};
            MPI_Recv(&partialCount, 1, MPI_LONG_LONG, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);
            totalCount += partialCount;

            int worker = status.MPI_SOURCE;
            if (nextRow < M) {
                int rowsToSend = std::min(DYNAMIC_CHUNK_ROWS, M - nextRow);
                sendTask(worker, nextRow, rowsToSend);
                nextRow += rowsToSend;
            } else {
                sendTask(worker, 0, 0);
                --activeWorkers;
            }
        }
    } else {
        // Worker: receive tasks, generate data locally, and process
        while (true) {
            int buf[2] = {0,0};
            MPI_Recv(buf, 2, MPI_INT, 0, TAG_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int startRow = buf[0];
            int rowCount = buf[1];

            if (rowCount <= 0) {
                break;
            }

            // Generate the requested rows locally (measure per-chunk time)
            double t0 = MPI_Wtime();
            long long localCount = countPrimesInRowRange(startRow, rowCount, R, seed);
            double t1 = MPI_Wtime();
            localTimeMs += (t1 - t0) * 1000.0;

            MPI_Send(&localCount, 1, MPI_LONG_LONG, 0, TAG_RESULT, MPI_COMM_WORLD);
        }
    }

    double processEnd = MPI_Wtime();

    // Reduce timing statistics across processes (max/min/avg)
    double maxTimeMs = 0.0;
    double minTimeMs = 0.0;
    double sumTimeMs = 0.0;
    MPI_Reduce(&localTimeMs, &maxTimeMs, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&localTimeMs, &minTimeMs, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&localTimeMs, &sumTimeMs, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double totalProcMs = (processEnd - processStart) * 1000.0;
        double avgTimeMs = sumTimeMs / size;
        std::cout << "-------------------------------------------------\n"
                  << " Prime numbers found : " << totalCount << "\n"
                  << " Processing time (wall) : " << totalProcMs << " ms\n"
                  << " Processing time (max)  : " << maxTimeMs << " ms\n"
                  << " Processing time (min)  : " << minTimeMs << " ms\n"
                  << " Processing time (avg)  : " << avgTimeMs << " ms\n"
                  << "-------------------------------------------------\n";
    }

    // No matrix to delete - generated locally per worker
    MPI_Finalize();
    return 0;
}
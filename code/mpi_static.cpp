/*
 * mpi_static.cpp - version MPI with static row distribution
 *
 * Compilation:
 *   mpic++ -o mpi_static mpi_static.cpp
 *
 * Usage:
 *   mpirun -np <P> ./mpi_static <M> <R> [seed]
 *
 *   <M>    : number of rows    (positive integer)
 *   <R>    : number of columns (positive integer)
 *   [seed] : optional random seed shared across versions
 *
 * Example:
 *   mpirun -np 4 ./mpi_static 1000 1000 12345
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
                  << " MPI STATIC VERSION - Prime count by rows\n"
                  << "-------------------------------------------------\n"
                  << " Rows    (M) : " << M << "\n"
                  << " Columns (R) : " << R << "\n"
                  << " Total elems  : " << static_cast<long long>(M) * R << "\n"
                  << " Value range  : [" << MIN_VAL << ", " << MAX_VAL << "]\n"
                  << " Processes    : " << size << "\n"
                  << " Seed         : " << seed << "\n"
                  << "-------------------------------------------------\n";
    }

    // Broadcast seed to all processes
    MPI_Bcast(&seed, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    // Calculate local row range for this process
    int baseRows = M / size;
    int extraRows = M % size;
    int localRows = baseRows + (rank < extraRows ? 1 : 0);
    int startRow = rank * baseRows + std::min(rank, extraRows);

    if (rank == 0) {
        std::cout << "[INFO] Building matrix..." << std::flush;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
           std::cout << " done. (processing on-the-fly)\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double processStart = MPI_Wtime();

    long long localCount = countPrimesInRowRange(startRow, localRows, R, seed);

    std::vector<long long> partialCounts;
    if (rank == 0) {
        partialCounts.resize(size, 0);
    }

    MPI_Gather(&localCount,
               1,
               MPI_LONG_LONG,
               rank == 0 ? partialCounts.data() : nullptr,
               1,
               MPI_LONG_LONG,
               0,
               MPI_COMM_WORLD);

    double processEnd = MPI_Wtime();

    if (rank == 0) {
        long long totalCount = 0;
        for (long long partial : partialCounts) {
            totalCount += partial;
        }

        std::cout << "-------------------------------------------------\n"
                  << " Prime numbers found : " << totalCount << "\n"
                  << " Processing time     : " << (processEnd - processStart) * 1000.0 << " ms\n"
                  << "-------------------------------------------------\n";
    }

    // No matrix to delete - data generated and processed on-the-fly
    MPI_Finalize();
    return 0;
}
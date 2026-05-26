/*
 * sequential.cpp — sequential version
 *
 * Compilation:
 *   mpic++ -o sequential sequential.cpp
 *
 * Usage:
 *   mpirun -np 1 ./sequential <M> <R>
 *
 *   <M>  : number of rows  (positive integer)
 *   <R>  : number of columns (positive integer)
 *
 * Example:
 *   mpirun -np 1 ./sequential 1000 1000
 */
 
#include <mpi.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
 
// Valor máximo que puede tomar un elemento de la matriz
static const int MAX_VAL = 1000000000;
 
bool isPrime(int n)
{
    if (n < 2) return false;          // 0 y 1 no son primos
    if (n == 2) return true;          // 2 es primo
    if (n % 2 == 0) return false;     // pares > 2 no son primos
 
    int raiz = static_cast<int>(std::sqrt(static_cast<double>(n)));
    for (int i = 3; i <= raiz; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}
 

int* buildMatrix(int M, int R) {
    int total = M * R;
    int* mat = new int[total];

    for (int k = 0; k < total; ++k) {
        mat[k] = (std::rand() % MAX_VAL) + 1;
    }

    return mat;
}
 
int main(int argc, char* argv[]){

    MPI_Init(&argc, &argv);
 
    if (argc != 3) {
        std::cerr << "\n[ERROR] Número incorrecto de argumentos.\n"
                  << "  Uso correcto : mpirun -np 1 " << argv[0] << " <M> <R>\n"
                  << "  Ejemplo      : mpirun -np 1 " << argv[0] << " 1000 1000\n\n";
        MPI_Finalize();
        return 1;
    }
 
    int M = std::atoi(argv[1]);
    int R = std::atoi(argv[2]);
 
    if (M <= 0 || R <= 0) {
        std::cerr << "\n[ERROR] Los tamaños M y R deben ser enteros positivos.\n"
                  << "  Recibido: M=" << argv[1] << "  R=" << argv[2] << "\n\n";
        MPI_Finalize();
        return 1;
    }
 
    // Confirmación de parámetros al usuario
    std::cout << "─────────────────────────────────────────────\n"
              << " Versión SECUENCIAL — Conteo de primos\n"
              << "─────────────────────────────────────────────\n"
              << " Filas    (M) : " << M << "\n"
              << " Columnas (R) : " << R << "\n"
              << " Total elem.  : " << static_cast<long long>(M) * R << "\n"
              << " Rango valores: [1, " << MAX_VAL << "]\n"
              << "─────────────────────────────────────────────\n";
 
    
    std::srand(static_cast<unsigned>(std::time(nullptr)));
 
    std::cout << "[INFO] Construyendo la matriz..." << std::flush;
    double t_build_inicio = MPI_Wtime();
 
    int* A = buildMatrix(M, R);
 
    double t_build_fin = MPI_Wtime();
    std::cout << " hecho. ("
              << (t_build_fin - t_build_inicio) * 1000.0 << " ms)\n";
 
    std::cout << "[INFO] Recorriendo la matriz y contando primos..." << std::flush;
    double t_inicio = MPI_Wtime();
 
    long long conteo = 0;
 
    for (int i = 0; i < M; i++) {           
        for (int j = 0; j < R; j++) { 
            // como la matriz se almacena en un arreglo unidimensional, multiplicamos el índice de fila por el número de columnas y sumamos el índice de columna
            // ejemplo si tenemos una matriz [123 456 789] con M=3 y R=3, el elemento A[1][2] (fila 1, columna 2) se encuentra en la posición 1*3 + 2 = 5 del arreglo unidimensional, es decir A[5] = 6     
            int valor = A[i * R + j];     
            if (isPrime(valor)) {
                conteo++;                    
            }
        }
    }
 
    double t_fin = MPI_Wtime();
    std::cout << " hecho.\n";
 
    // ── Resultados ─────────────────────────────────────────────────────────
    double tiempo_ms = (t_fin - t_inicio) * 1000.0;
 
    std::cout << "─────────────────────────────────────────────\n"
              << " Números primos encontrados : " << conteo << "\n"
              << " Tiempo de cómputo          : " << tiempo_ms << " ms\n"
              << "─────────────────────────────────────────────\n";
 
    // Liberación de memoria y cierre
    delete[] A;
    MPI_Finalize();
    return 0;
}

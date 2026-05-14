/*
 * sequential_chrono.cpp — versión secuencial sin MPI (usa std::chrono)
 *
 * Compilación:
 *   g++ -o sequential_chrono sequential_chrono.cpp
 *
 * Uso:
 *   ./sequential_chrono <M> <R>
 *
 *   <M>  : número de filas  (entero positivo)
 *   <R>  : número de columnas (entero positivo)
 *
 * Ejemplo:
 *   ./sequential_chrono 1000 1000
 */

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

// Valor máximo que puede tomar un elemento de la matriz
static const int MAX_VAL = 10000;

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

int* buildMatrix(int M, int R) {
    int total = M * R;
    int* mat = new int[total];

    for (int k = 0; k < total; ++k) {
        mat[k] = (std::rand() % MAX_VAL) + 1;
    }

    return mat;
}

int main(int argc, char* argv[]){

#ifdef _WIN32
    // Forzar la consola de Windows a UTF-8 para evitar caracteres corruptos
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc != 3) {
        std::cerr << "\n[ERROR] Número incorrecto de argumentos.\n"
                  << "  Uso correcto : " << argv[0] << " <M> <R>\n"
                  << "  Ejemplo      : " << argv[0] << " 1000 1000\n\n";
        return 1;
    }

    int M = std::atoi(argv[1]);
    int R = std::atoi(argv[2]);

    if (M <= 0 || R <= 0) {
        std::cerr << "\n[ERROR] Los tamaños M y R deben ser enteros positivos.\n"
                  << "  Recibido: M=" << argv[1] << "  R=" << argv[2] << "\n\n";
        return 1;
    }

    // Confirmación de parámetros al usuario
    std::cout << "─────────────────────────────────────────────\n"
              << " Versión SECUENCIAL — Conteo de primos (chrono)\n"
              << "─────────────────────────────────────────────\n"
              << " Filas    (M) : " << M << "\n"
              << " Columnas (R) : " << R << "\n"
              << " Total elem.  : " << static_cast<long long>(M) * R << "\n"
              << " Rango valores: [1, " << MAX_VAL << "]\n"
              << "─────────────────────────────────────────────\n";

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::cout << "[INFO] Construyendo la matriz..." << std::flush;
    auto t_build_inicio = std::chrono::high_resolution_clock::now();

    int* A = buildMatrix(M, R);

    auto t_build_fin = std::chrono::high_resolution_clock::now();
    double build_ms = std::chrono::duration<double, std::milli>(t_build_fin - t_build_inicio).count();
    std::cout << " hecho. (" << build_ms << " ms)\n";

    std::cout << "[INFO] Recorriendo la matriz y contando primos..." << std::flush;
    auto t_inicio = std::chrono::high_resolution_clock::now();

    long long conteo = 0;

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < R; j++) {
            int valor = A[i * R + j];
            if (isPrime(valor)) {
                conteo++;
            }
        }
    }

    auto t_fin = std::chrono::high_resolution_clock::now();
    std::cout << " hecho.\n";

    double tiempo_ms = std::chrono::duration<double, std::milli>(t_fin - t_inicio).count();

    std::cout << "─────────────────────────────────────────────\n"
              << " Números primos encontrados : " << conteo << "\n"
              << " Tiempo de cómputo          : " << tiempo_ms << " ms\n"
              << "─────────────────────────────────────────────\n";

    delete[] A;
    return 0;
}

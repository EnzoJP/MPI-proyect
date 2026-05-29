#!/bin/bash
#SBATCH --job-name=SeqPrimes
#SBATCH --ntasks=1
#SBATCH --time=00:30:00
#SBATCH --array=1-2
#SBATCH --output=/dev/null                # Le decimos a Slurm que no cree archivos .out

module purge
module load gcc/13.2.0
module load openmpi/4.1.5

ARCHIVO_UNICO="sequential_results_100kx10k.out"

# Usamos { ... } >> archivo.out para meter todos los prints de esta tarea en el mismo archivo compartido
{
    echo "=== INICIO TAREA ARRAY ID: $SLURM_ARRAY_TASK_ID (Semilla: $SEMILLA) ==="
    srun ./sequential 100000 10000 12345
    echo "=== FIN TAREA ARRAY ID: $SLURM_ARRAY_TASK_ID ==="
} >> $ARCHIVO_UNICO

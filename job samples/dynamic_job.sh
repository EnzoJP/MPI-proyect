#!/bin/bash
#SBATCH --job-name=DynPrim
#SBATCH --ntasks=4                # 4 CPUs por cada una de las 30 ejecuciones
#SBATCH --time=00:30:00 

# Directiva mágica: crea 30 tareas numeradas del 1 al 30
#SBATCH --array=1-30

# El token %A se reemplaza por el ID del Job principal, y %a por el número de la repetición (1 al 30)
#SBATCH --output=/dev/null

# --- Ejecución ---
module purge
module load gcc
module load openmpi

# Podés usar la variable $SLURM_ARRAY_TASK_ID si necesitás que cada corrida 
# use un parámetro distinto (ej: parametros_$SLURM_ARRAY_TASK_ID)
ARCHIVO_UNICO="dynamic_results_10x10_4np_8.out"

# Usamos { ... } >> archivo.out para meter todos los prints de esta tarea en el mismo archivo compartido
{
    echo "=== INICIO TAREA ARRAY ID: $SLURM_ARRAY_TASK_ID (Semilla: $SEMILLA) ==="
    srun --ntasks=4 ./mpi_dynamic 10000 10000 12345
    echo "=== FIN TAREA ARRAY ID: $SLURM_ARRAY_TASK_ID ==="
} >> $ARCHIVO_UNICO

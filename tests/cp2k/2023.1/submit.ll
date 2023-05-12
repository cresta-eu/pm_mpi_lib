#!/bin/bash --login
  
#SBATCH --job-name=cp2k
#SBATCH --nodes=8
#SBATCH --tasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --time=08:00:00
#SBATCH --account=z19
#SBATCH --partition=standard
#SBATCH --qos=standard

export MPICH_VERSION_DISPLAY=1
export MPICH_ENV_DISPLAY=1
export MPICH_RANK_REORDER_DISPLAY=1
export MPICH_CPUMASK_DISPLAY=1

export OMP_NUM_THREADS=1

srun --distribution=block:block --hint=nomultithread --unbuffered ./cp2k.psmp.pm -i H2O-1024.inp >& stdouterr

#!/bin/bash --login
  
#SBATCH --job-name=dl_poly
#SBATCH --nodes=4
#SBATCH --tasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --time=01:00:00
#SBATCH --account=z19
#SBATCH --partition=standard
#SBATCH --qos=standard

jobdir="job01"
cd ${jobdir}

export MPICH_VERSION_DISPLAY=1
export MPICH_ENV_DISPLAY=1
export MPICH_RANK_REORDER_DISPLAY=1
export MPICH_CPUMASK_DISPLAY=1

export OMP_NUM_THREADS=1

srun --distribution=block:block --hint=nomultithread --unbuffered ./DLPOLY.Z >& stdouterr

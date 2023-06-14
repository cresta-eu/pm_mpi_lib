#!/bin/bash --login
  
#SBATCH --job-name=pm
#SBATCH --nodes=4
#SBATCH --tasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --account=[budget code]
#SBATCH --partition=standard
#SBATCH --qos=short


module -q restore
module -q load cpe/22.12
module -q load PrgEnv-cray


export OMP_NUM_THREADS=1

mkdir -p ${SLURM_SUBMIT_DIR}/pmc
lfs setstripe -c -1 ${SLURM_SUBMIT_DIR}/pmc

srun --distribution=block:block --hint=nomultithread --unbuffered ./pm_mpi_test 

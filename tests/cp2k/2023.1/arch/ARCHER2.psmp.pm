CC       = cc -fopenmp
CXX      = CC -fopenmp
FC       = ftn -fopenmp
LD       = ftn -fopenmp
AR       = ar -r

CP2K_ROOT = /work/z19/z19/mrb23cab/apps/cp2k/cp2k-2023.1
DATA_DIR = $(CP2K_ROOT)/data

LIBINT_VERSION         = 2.6.0
LIBINT_VERSION_SUFFIX  = cp2k-lmax-4
LIBXC_VERSION          = 6.1.0
ELPA_VERSION           = 2022.11.001
PLUMED_VERSION         = 2.8.2
PM_MPI_LIB_VERSION     = 4.4.0

LIBINT_ROOT            = $(CP2K_ROOT)/libs/libint/$(LIBINT_VERSION)/$(LIBINT_VERSION_SUFFIX)
LIBXC_ROOT             = $(CP2K_ROOT)/libs/libxc/$(LIBXC_VERSION)
ELPA_ROOT              = $(CP2K_ROOT)/libs/elpa/$(ELPA_VERSION)/openmp
PLUMED_ROOT            = $(CP2K_ROOT)/libs/plumed/$(PLUMED_VERSION)
PM_MPI_LIB_ROOT        = $(CP2K_ROOT)/libs/pm_mpi_lib/releases/v$(PM_MPI_LIB_VERSION)


# Provides PLUMED_DEPENDENCIES

include $(PLUMED_ROOT)/lib/plumed/src/lib/Plumed.inc.static


# Options

DFLAGS   = -D__FFTW3 -D__LIBXC -D__PLUMED2  \
           -D__ELPA -D__LIBINT -D__MKL \
           -D__parallel -D__SCALAPACK -D__MPI_VERSION=3

WFLAGS      = -Werror=aliasing -Werror=ampersand -Werror=c-binding-type \
              -Werror=intrinsic-shadow -Werror=intrinsics-std -Werror=line-truncation \
              -Werror=tabs -Werror=target-lifetime -Werror=underflow -Werror=unused-but-set-variable \
              -Werror=unused-variable -Werror=unused-dummy-argument -Werror=conversion \
              -Werror=zerotrip -Wno-maybe-uninitialized -Wuninitialized -Wuse-without-only



CFLAGS   = -march=native -mtune=native -fno-omit-frame-pointer -g -O3 -funroll-loops \
           -I$(LIBINT_ROOT)/include  \
           -I$(LIBXC_ROOT)/include  \
           -I${MKLROOT}/include -m64 \
           -I$(ELPA_ROOT)/include/elpa_openmp-$(ELPA_VERSION)/modules \
           -I$(ELPA_ROOT)/include/elpa_openmp-$(ELPA_VERSION)/elpa \
           -I$(FFTW_INC) \
           -std=c11 -Wall -Wextra -Werror -Wno-vla-parameter -Wno-deprecated-declarations $(DFLAGS)


CXXFLAGS = -O2 -fPIC -fno-omit-frame-pointer -fopenmp -g -march=native -mtune=native --std=c++11 $(DFLAGS) -Wno-deprecated-declarations

FCFLAGS  = $(DFLAGS) $(WFLAGS) \
           -march=native -mtune=native -fno-omit-frame-pointer -g -O3 -funroll-loops \
           -I$(PM_MPI_LIB_ROOT)/mod/gnu \
	   -I$(LIBINT_ROOT)/include  \
           -I$(LIBXC_ROOT)/include  \
           -I${MKLROOT}/include -m64 \
           -I$(ELPA_ROOT)/include/elpa_openmp-$(ELPA_VERSION)/modules \
           -I$(ELPA_ROOT)/include/elpa_openmp-$(ELPA_VERSION)/elpa \
           -I$(FFTW_INC) -ffree-form -fbacktrace -fimplicit-none -std=f2008 -ffree-line-length-none -fallow-argument-mismatch

LDFLAGS  = $(FCFLAGS)  

LIBS     = -L$(PM_MPI_LIB_ROOT)/lib/gnu -lpmmpi \
           -L$(LIBINT_ROOT)/lib -lint2  \
           -L$(LIBXC_ROOT)/lib -lxcf90 -lxcf03 -lxc \
           -L$(ELPA_ROOT)/lib -lelpa_openmp \
           $(PLUMED_DEPENDENCIES) -lz \
           -L$(FFTW_LIB) -lfftw3 -lfftw3_threads \
           ${MKLROOT}/lib/intel64/libmkl_scalapack_lp64.a -Wl,--start-group \
           ${MKLROOT}/lib/intel64/libmkl_gf_lp64.a ${MKLROOT}/lib/intel64/libmkl_gnu_thread.a \
           ${MKLROOT}/lib/intel64/libmkl_core.a ${MKLROOT}/lib/intel64/libmkl_blacs_intelmpi_lp64.a \
           -Wl,--end-group -lgomp -lpthread -lm -ldl \
           -ldl -lstdc++ -lpthread

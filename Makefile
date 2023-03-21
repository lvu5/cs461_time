#_______________________________________________________________________________
# ethos make: the ethos project contains source that is built for different
# architectures, different environments, and different languages.  Eventually,
# it will target different distributions, although for now it is Fedora based.
#
# Ethos Makefile:  Ethos contains one non-recursive Makefile for building
# the kernel, libraries, languages, packages, userspace and linux programs.
# The rationale behind this design is in  miller97recursiveMake
# 
#
# TARGET_ARCH is x86_64 or x86_32
# environment is either
#     k - kernel
#

#  Common tasks
#	uninstall:             sudo -E make uninstall
#       build src and install: make && sudo -E make install
#	run and check tests:   make build && sudo -E make install && make check
#
#
#
# Building RPMs:
#
# The rpm.build target will build all rpm packages the system contains. Individual packages
# can be built by specifying the package name at the end.
# For example, to build nacl, type 	make rpm.build.nacl
# For a list of available packages please refer to mk/ which contains all the .spec files.
# Please note that rpm-build needs to be installed.
#
# Running the static analyzer:
# 
# The static-check target will build with the llvm scan-build tool and generate a html report.
# This needs the clang-analyzer package.
#
#_______________________________________________________________________________

.DEFAULT_GOAL := all


# sets TARGET_ARCH and compiler flags
include top.mk
include install.mk

ETHOSROOTDIR := $(abspath .)
GITHOST      := git.ethos-os.org
STAGEDIR     := $(ETHOSROOTDIR)/destStageDir
PATH         := $(PATH):$(ETHOSROOTDIR)/bin:$(RPM_BUILD_ROOT)/usr/bin:$(RPM_BUILD_ROOT)/usr/sbin

ethos.git.dir  := /home/git/ethos.d



build:    test.build


clean:     
	rm -rf kobjs-x86_64

wc:       
	cloc $(SRC)

static-check:
	make clean
	scan-build make WITH_ASSERTS=y

valgrind-clean:
	rm -rf lobjs-x86_32/test/test/* lobjs-x86_64/test/test/* uobjs-x86_32/test/test/* uobjs-x86_64/test/test/*

valgrind-run: $(test.libtest.target) test.test.minimaltd.build test.test.streamingDirectory.build
	minimaltdPrefix="valgrind --leak-check=full --log-file=valgrind.log " sudo -E test/scripts/minimaltdTestRun $(LDIR)/$(test.test.minimaltd.dir) client server
	shadowdaemonPrefix="valgrind --log-file=valgrind.log" sudo -E test/scripts/testRun $(UDIR) $(test.test.streamingDirectory.dir) client


#	debug/debug.o\

kernel.objects.coded =\
	arch/$(TARGET_ARCH)/time.o\
	arch/$(TARGET_ARCH)/traps.o\
	arch/$(TARGET_ARCH)/trapsSpecific.o\
	arch/$(TARGET_ARCH)/archInit.o\
	arch/$(TARGET_ARCH)/archPageTable.o\
	arch/$(TARGET_ARCH)/archPageTableSpecific.o\
	arch/$(TARGET_ARCH)/archKernelBlock.o\
	arch/$(TARGET_ARCH)/archDebug.o\
	hw2/my_mem.o\
	hw2/my_printf.o\
	hw2/my_str.o\
	hw2/my_malloc.o\
	xen/xenSchedule.o\
	xen/xenbus.o\
	xen/xenPageTable.o\
	xen/xenEvent.o\
	xen/xenEventHandler.o\
	xen/xenGrant.o\
	version.o\
	src/initialStore.o\
	src/xencons_ring.o\
	src/console.o\
	src/handle.o\
	src/debug.o\
	src/mixin.o\
	src/print.o\
	src/traditionalSyscallHandler.o\
	src/startKernel.o

kernel.objects := $(kernel.objects.coded)

kernel.libmem.objects =\
	libmem/pageTable.o\
	libmem/vaddr.o\
	libmem/pageFault.o\
	libmem/virtualInfo.o\
	libmem/physicalInfo.o

kernel.headers :=\
	include/nano/arch/bits.h \
	include/nano/arch/cpuPrivileged.h \
	include/nano/arch/archDebug.h \
	include/nano/arch/elf.h \
	include/nano/arch/hypercall.h \
	include/nano/arch/ldsyms.h \
	include/nano/arch/memory.h \
	include/nano/arch/archPageTable.h \
	include/nano/arch/schedPrivileged.h \
	include/nano/arch/synchro.h \
	include/nano/x86_32/bits.h \
	include/nano/x86_32/cpuPrivileged.h \
	include/nano/x86_32/elf.h \
	include/nano/x86_32/hypercall.h \
	include/nano/x86_32/memory.h \
	include/nano/x86_32/archPageTable.h \
	include/nano/x86_32/synchro.h \
	include/nano/x86_64/bits.h \
	include/nano/x86_64/cpuPrivileged.h \
	include/nano/x86_64/elf.h \
	include/nano/x86_64/hypercall.h \
	include/nano/x86_64/memory.h \
	include/nano/x86_64/archPageTable.h \
	include/nano/x86_64/synchro.h \
	include/nano/arg.h \
	include/nano/blkfront.h \
	include/nano/common.h \
	include/nano/console.h \
	include/nano/debug.h \
	include/nano/efs.h \
	include/nano/elf.h \
	include/nano/eventProcess.h \
	include/nano/fileSystem.h \
	include/nano/initialStore.h \
	include/nano/kernelLog.h\
	include/nano/latch.h \
	include/nano/mm.h \
	include/nano/mm_public.h \
	include/nano/pageTable.h \
	include/nano/permission.h \
	include/nano/physicalInfo.h \
	include/nano/process.h \
	include/nano/processMemory.h \
	include/nano/processMemoryRegion.h \
	include/nano/resource.h \
	include/nano/syscall.h \
	include/nano/time.h \
	include/nano/timer.h \
	include/nano/userspace.h \
	include/nano/version.h \
	include/nano/xenbus.h \
	include/nano/xenEvent.h \
	include/nano/xenEventHandler.h \
	include/nano/xenGrant.h \
	include/nano/xenPageTable.h \
	include/nano/xenSchedule.h

# Prefix for global API names. All other symbols are localised before
# linking with EXTRA_OBJS.
GLOBAL_PREFIX := xenos_

KERNEL=nanoOS.elf

kernel.dir           := .
kernel.header.dir    := include
kernel.build         := $(KDIR)
kernel.objects.build := $(patsubst %, $(kernel.build)/%, $(kernel.objects))
kernel.target        := $(kernel.build)/$(KERNEL)

kernel.entry := $(KDIR)/arch/$(TARGET_ARCH)/entry.o
kernel.libmem.build :=  $(patsubst %, $(kernel.build)/%, $(kernel.libmem.objects))

KOBJS   += $(kernel.objects.build) $(kernel.libmem.build)

KASOBJS += $(kernel.entry)
kernel.src := $(patsubst %, include/%, $(kernel.headers)) $(patsubst %, %, $(kernel.objects.coded:.o=*.c))
SRC     += $(kernel.src)

kernel.lib = lib/libethos.a
kernel.libmem.target = lib/libmem.a

all : $(kernel.target)

#$(kernel.libmem.target):  $(kernel.libmem.build)
#	$(AR) rc $(kernel.libmem.target) $(kernel.libmem.build)

# note kernel.entry must be first loaded object file
$(kernel.target): $(kernel.entry) $(kernel.objects.build) $(kernel.libmem.target)
	$(LD) $(K_LDFLAGS) -r $(kernel.entry) $(kernel.objects.build) $(kernel.libmem.target) $(kernel.lib) -o $(kernel.target).o
	objcopy -w -G xenos_* -G _start  $(kernel.target).o $(kernel.target).o
	ld $(K_LDFLAGS) -N -T $(ethos.kernel.script) $(kernel.target).o -o $(kernel.target)


kernel.clean:


distclean:

include bottom.mk

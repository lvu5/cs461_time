#_______________________________________________________________________________
# top
#_______________________________________________________________________________


#---------------- Explicitly CANCEL EVIL BUILTIN RULES:
%.o : %.c
%.o : %.s
%.c : %.l
%.c : %.y
%.r : %.l

# top level in which binaries are built
BUILD_DIRS = kobjs-x86_64


# if TARGET_ARCH not set, go with x86_64
TARGET_ARCH     ?= x86_64
TARGET_ARCH_FAM  = x86

WITH_DEBUG   ?= n
WITH_PROFILE ?= n
WITH_ASSERTS ?= y
WITHOUT_OPT  ?= n

ARFLAGS = cr # archive field

KDIR = kobjs-$(TARGET_ARCH)

ethos.types=$(dual.libkernelTypes.ar)

kernelTypesDir=dual/libkernelTypes/kernelTypesIndex

ethos.kernel.script=arch/$(TARGET_ARCH)/kernel.script
ethos.ld.script=mk/$(TARGET_ARCH)/ld.script
ethos.crt.script=mk/$(TARGET_ARCH)/crt.script

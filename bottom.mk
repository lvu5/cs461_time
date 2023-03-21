#_______________________________________________________________________________
# bottom.mk: these are defined at the bottom because they depend on objects
# defined throughout the tree.
#_______________________________________________________________________________

# NOTE: removed -Winline because GCC 4.6 warns that lot's of function will not be
#       inlined with optimizations. Let's just let -O3 do its thing.
#
#       -fno-optimize-sibling-calls is for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=48385
#       This GCC bug (?) affects the 64-bit build.
#
#       -Wno-strict-aliasing for Fedora 14 GCC (remove once migrated to 15+?).
XEN_INTERFACE_VERSION := 0x00040300
BASE_CPPFLAGS  = -D__$(TARGET_ARCH)__

# These are the default CFLAGS for ALL builds, kernel, user space, Linux, or Ethos.
# gnu99, not c99, is required for using Linux list data structure (typeof)

GNU_SEMANTICS=gnu99

BASE_CFLAGS += \
        -std=$(GNU_SEMANTICS) \
        -fno-optimize-sibling-calls \
        -fno-stack-protector \
        -Wall \
        -Wnested-externs \
        -Werror \
	-static \
	-fno-builtin \
	-fno-leading-underscore \

# FIXME: Use gnu89 instead of gnu99 for eg2source-generated C.
#        Gets us compound literals as constant expressions.
#        Ethos build default is -gnu99, but this will follow, overriding.
#        Presently required for types (fix C code generator there).
#        See also: types/runtimeC/libetn/lib/Makefile
#        See also: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=7946
#BASE_CFLAGS += -std=gnu99


# WITH_ASSERTS is for Ethos kernel code
# NDEBUG is for POSIX assert (see man assert), although we should use ASSERT/doAssert.
ifeq ($(WITH_ASSERTS),y)
  BASE_CPPFLAGS += -DWITH_ASSERTS
else
  BASE_CPPFLAGS += -DNDEBUG
endif

# add in debug flag if requested
ifeq ($(WITH_DEBUG),y)
  BASE_CFLAGS   += -g
  BASE_CPPFLAGS += -DDEBUG
endif

# add in valgrind flag if requested
ifeq ($(WITH_VALGRIND),y)
  BASE_CPPFLAGS += -DWITH_VALGRIND
endif

# add in profile flag if requested
ifeq ($(WITH_PROFILE),y)
  BASE_CFLAGS   += -p
  BASE_CPPFLAGS += -DDEBUG
endif

ifeq ($(WITH_GPROF),y)
  BASE_CFLAGS += -pg
endif

# add in optimization by default
ifeq ($(WITHOUT_OPT),n)
  BASE_CFLAGS += -O3
endif


AR_FLAGS ?= -s -r -c

K_CFLAGS += -nostdlib -ffreestanding
U_CFLAGS += -nostdlib -ffreestanding

# all includes in dual added
DUAL_LIBS        :=  adt  core debug event fmt string utf xalloc
BASE_CPPFLAGS    +=  
K_CPPFLAGS        =  -D__XEN_INTERFACE_VERSION__=$(XEN_INTERFACE_VERSION) $(BASE_CPPFLAGS) -I include  # shouldn't be used for dual build

#_______________________________________________________________________________
#  architecturally dependent flags
#_______________________________________________________________________________
ifeq ($(TARGET_ARCH),x86_64)
  BASE_ASFLAGS += -m64 -D__x86_64__ -D__ASSEMBLY__
  BASE_CFLAGS  += -m64 -mcmodel=large

  # gcc tries to use XMM registers for (e.g.) memset when using -O3.
  # We must unset MMX, etc. so that this does not happen,
  # because using XMM registers in the kernel without a fxsave is bad.
  # This has the side effect that you cannot return a (e.g.) double on x86_64.
  # this is only needed in 64-bit because our 32-bit target is a pre-MMX i386
  NOXMM_CFLAGS := -mno-mmx -mno-3dnow
  K_ASFLAGS += $(BASE_ASFLAGS)
  K_LDFLAGS += -static
  K_CFLAGS  += $(BASE_CFLAGS) -mno-red-zone -fno-reorder-blocks -fno-asynchronous-unwind-tables $(NOXMM_CFLAGS)

  U_ASFLAGS  += $(BASE_ASFLAGS)
  U_CFLAGS   += $(BASE_CFLAGS) -I userspace/libe/include -I test/libtest
  U_LDFLAGS  += 

  L_ASFLAGS += $(BASE_ASFLAGS)
  L_LDFLAGS += 
  L_CFLAGS  += $(BASE_CFLAGS) -I linux/libethosPosix/include

else

ifeq ($(TARGET_ARCH),x86_32)
  BASE_ASFLAGS += -m32 -D__x86_32__ -D__ASSEMBLY__
  BASE_CFLAGS  += -march=i686 -m32
  BASE_LDFLAGS += -melf_i386

  K_ASFLAGS += $(BASE_ASFLAGS)
  K_CFLAGS  += $(BASE_CFLAGS) $(BASE_INCLUDE_DIR) $(K_INCLUDE_DIR)
  K_LDFLAGS += -static $(BASE_LDFLAGS)

  U_ASFLAGS += $(BASE_ASFLAGS)
  U_CFLAGS  += $(BASE_CFLAGS) $(BASE_INCLUDE_DIR) -I userspace/libe/include -I test/libtest
  U_LDFLAGS += -melf_i386

  L_ASFLAGS += $(BASE_ASFLAGS)
  L_CFLAGS  += $(BASE_CFLAGS) $(BASE_INCLUDE_DIR) -I linux/libethosPosix/include
  L_LDFLAGS += -march=i686 -m32

else
  $(error unknown architecture: $(TARGET_ARCH))
endif
endif


L_LDLIBS  := $(LDIR)/linux/libethosPosix/lib/libethosPosix.a -lc 



$(DOBJS): $(KDIR)/%.o: %.c  dual/libcore/include/ethos/fdType.h dual/libcore/include/ethos/status.h dual/libcore/include/ethos/syscallId.h 
	@mkdir -p $(@D)
	@$(CC) -MM -MG  $(K_CPPFLAGS) -MT '$@' $< > $(patsubst %.o, $(KDIR)/%.d, $(subst /,., $@))
	$(CC) $(K_CFLAGS) $(K_CPPFLAGS) -c $< -o $@


$(KOBJS): $(KDIR)/%.o: %.c 
	@mkdir -p $(@D)
	@$(CC) -MM -MG  $(K_CPPFLAGS) -MT '$@' $< > $(patsubst %.o, $(KDIR)/%.d, $(subst /,., $@))
	$(CC) $(K_CFLAGS) $(K_CPPFLAGS) -c $< -o $@

$(KASOBJS): $(KDIR)/%.o: %.S 
	@mkdir -p $(@D)
	@$(CC) -MM -MG  $(K_CPPFLAGS) -MT '$@' $< > $(patsubst %.o, $(KDIR)/%.d, $(subst /,., $@))
	$(CC)  -g $(K_ASFLAGS) $(K_CPPFLAGS) -c $< -o $@


-include $(KDIR)/*.d

#_______________________________________________________________________________
# if we put distribution dependent install functions, here is where
# it should go
#_______________________________________________________________________________

install.kernel          = $(DESTDIR)/var/lib/xen/images/ethos.$(TARGET_ARCH).elf

                        # should be ethos-x86_64
install.nanoOS.header    = $(DESTDIR)/usr/include/ethos
install.nanoOS.lib    	= $(DESTDIR)/usr/$(TARGET_ARCH)-xen-ethos/lib
install.nanoOS.share    	= $(DESTDIR)/usr/$(TARGET_ARCH)-xen-ethos/share

install.rootfs.name        = nanoOS-$(TARGET_ARCH)
install.rootfs.parent      = $(DESTDIR)/var/lib/ethos/$(install.rootfs.name)
install.rootfs             = $(install.rootfs.parent)/rootfs
install.typeHashes.all     = $(install.rootfs)/types/all
install.typeHashes.spec    = $(install.rootfs)/types/spec

install.bin		= $(DESTDIR)/usr/bin
install.sbin		= $(DESTDIR)/usr/sbin
install.share		= $(DESTDIR)/usr/share

# ethos installs
INSTALL_ETHOSROOT_PROGRAMS=
INSTALL_ETHOSROOT_VIRTUALPROGRAMS =
INSTALL_ETHOSROOT_SYSTEM_PROGRAMS=


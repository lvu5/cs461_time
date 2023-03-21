//______________________________________________________________________________
// xtypes used in the kernel
//
//______________________________________________________________________________

#ifndef __ETHOSXTYPE_H__
#define __ETHOSXTYPE_H__

// abstractions
Xtype eventXtype;    
Xtype resourceXtype;
Xtype eventIdXtype;
Xtype eventWaitTreeXtype;
Xtype handleTableEntryXtype;
Xtype rdIdXtype;
Xtype listHeadXtype;

// certificate
Xtype directoryCertificateXtype;
Xtype ephemeralHostCertificateXtype;
Xtype hostCertificateXtype;
Xtype hostGroupCertificateXtype;
Xtype aliasCertificateXtype;
Xtype userCertificateXtype;
Xtype userGroupCertificateXtype;
Xtype encryptionKeyPairXtype;

// auth
Xtype authenticatorXtype;
Xtype userRecordXtype;
Xtype authUserXtype;
Xtype authenticatorNodeXtype;
Xtype authExecutableUserXtype;
Xtype userAuthenticationXtype;

// files
Xtype resourceDescriptorXtype;
Xtype resourceDescriptorListXtype;
Xtype fileInformationXtype;
Xtype typeNodeXtype;
Xtype typeGraphHashNodeXtype;
// fsFrontInterfaceXtype to store the xen fs front end interface
Xtype blkfrontInterfaceXtype;

// ipc
Xtype advertiseXtype;
Xtype ipcXtype;
Xtype ipcWaitingWriteXtype;

// mm
Xtype kernelPageXtype;
Xtype userSpacePageXtype;
Xtype kmapXtype;
Xtype physicalInfoXtype;
Xtype availPhysicalXtype;
Xtype processMemoryXtype;
Xtype processMemoryRegionXtype;
Xtype virtualInfoXtype;
Xtype virtualAddressXtype;

// netInterfaceXtype to store the network interfaces.
Xtype netInterfaceXtype;

// Process xtype Id for allocating process structures.
Xtype processXtype;
Xtype exitStatusXtype;
Xtype pendingFdXtype;

// FloatingPointStore xtype for xallocation
Xtype floatingPointStoreXtype;

// other
Xtype intXtype;
Xtype ptrXtype;
#endif

//______________________________________________________________________________
/// Versioning information.
// Mar-2008: Andrei Warkentin
//______________________________________________________________________________

#ifndef __VERSION_H__
#define __VERSION_H__


// see vesion.c for setting and meaning of different
extern const uint EthosMajorVersion;
extern const uint EthosMinorVersion;
extern const uint EthosSubMinorVersion;
extern const char ethosPrintableVersion[];

#define VERSION_SYMBOL kernel_version
extern const char kernel_version[];

#endif

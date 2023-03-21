//______________________________________________________________________________
// Mixin functions;
// Oct-2011: W. Michael Petullo
//______________________________________________________________________________

#ifndef __MIXIN_H__
#define __MIXIN_H__

#include <nano/status.h>
#include <nano/ethosTypes.h>
#include <nano/slice.h>
#include <nano/mixinBase.h>
#include <nano/ref.h>

Status  mixinDirectoryClean(String *path);
Status  mixinGetRandomBytes(uchar *x, unsigned long long xlen);
String **mixinSubFiles(String *pathvec[]);

#endif

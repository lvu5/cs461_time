//______________________________________________________________________________
//  command line arguments to the kernel
//______________________________________________________________________________
#ifndef __ARG_H__
#define __ARG_H__

void  kernelArgPrint(void);
char* kernelArg(const char *key);
void  kernelArgInit(char *cmdLine);

#endif

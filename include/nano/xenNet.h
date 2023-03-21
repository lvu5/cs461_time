//______________________________________________________________________________
/// Xen networking functions
//
// October 2013: W. Michael Petullo
//______________________________________________________________________________

#ifndef __XEN_NET_H__
#define __XEN_NET_H__

int  xenNetIncoming(void);
bool xenNetSend(Packet *packet);
bool xenNetSendFull(unsigned int interface);
bool xenNetGetSrcMac(EthernetMac mac, int interface);
void xenNetInit(void);
void xenNetHandlerDeferred(int i);

#endif //__XEN_NET_H__

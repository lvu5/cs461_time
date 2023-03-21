/* Adopted from lkfront.h in Mini-OS */
#ifndef __XEN_FS_H__
#define __XEN_FS_H__

#include <ethos/kernel/arch/synchro.h>
#include <xen/io/blkif.h>

struct blkfront_info {
	uint64 sectors;
	unsigned sector_size;
	int mode;
	int info;
	int barrier;
	int flush;
};

typedef struct BlkfrontInterface {
	unsigned backendid;  // backend domain id, most likely 0
	char *backend; // backend path as c string
	struct {  // the ring
		blkif_sring_t *sring;
		blkif_front_ring_t front;
		blkif_back_ring_t back;
		grant_ref_t ring_ref;
	} ring;
	unsigned evtchn;  // event channel
	blkif_vdev_t handle;
	struct blkfront_info info;
} BlkfrontInterface;

struct blkfront_aiocb {
	BlkfrontInterface *iface;
	uint8_t *aio_buf;
	size_t aio_nbytes;
	uint64 aio_offset;
	int is_write;
	int is_done;
	unsigned long id; // index into efsReplyInfoArray

	grant_ref_t gref[BLKIF_MAX_SEGMENTS_PER_REQUEST];
	int n; // how many pages to IO
};

void blkfrontInit(void);

Status blkfrontDiskOp(int write, unsigned int sector, int nsec, Ref *buf, unsigned long id);
void blkfrontHandlerDeferred(void);
void efsInit(void);

#endif //__XEN_FS_H__

//______________________________________________________________________________
/// Xenbus/XenStore code.
/// XenStore is a hierarchical store for holding strings
/// XenBus is used to communicate with device drivers
//
// mini-OS derived
// May-2008: Andrei Warkentin
//______________________________________________________________________________

#include <nano/common.h>
#include <nano/xenbus.h>
#include <nano/xenEvent.h>
#include <nano/schedPrivileged.h>
#include <nano/fmt.h>

// Request id table.
static HandleTable reqIds;

// XenStore messages.
typedef enum
{
    XS_DEBUG,
    XS_DIRECTORY,
    XS_READ,
    XS_GET_PERMS,
    XS_WATCH,
    XS_UNWATCH,
    XS_TRANSACTION_START,
    XS_TRANSACTION_END,
    XS_INTRODUCE,
    XS_RELEASE,
    XS_GET_DOMAIN_PATH,
    XS_WRITE,
    XS_MKDIR,
    XS_RM,
    XS_SET_PERMS,
    XS_WATCH_EVENT,
    XS_ERROR,
    XS_IS_DOMAIN_INTRODUCED,
    XS_RESUME
} xsd_sockmsg_type_t;

#define XS_WRITE_NONE "NONE"
#define XS_WRITE_CREATE "CREATE"
#define XS_WRITE_CREATE_EXCL "CREATE|EXCL"

// Max...
#define XENBUS_PRINTF_SIZE 4096

// We have errors as strings, for portability.
struct xsd_errors
{
    int errnum;
    const char *errstring;
};

// XenStore messages.
typedef struct
{
    uint32_t type;   // XS_???
    uint32_t req_id; // Request identifier, echoed in daemon's response.
    uint32_t tx_id;  // Transaction id (0 if not related to a transaction).
    uint32_t len;    // Length of data following this.

  // Generally followed by null-terminated string(s).
} xs_message_t;

enum xs_watch_type
    {
	XS_WATCH_PATH = 0,
	XS_WATCH_TOKEN
    };

// XS write req.
typedef struct
{
    const void *data;
    unsigned len;
} xs_write_req_t;

// Reply structure.
typedef struct
{
    enum { RECEIVED, PENDING } state;
    void *reply;
} xs_write_reply_t;

// Global that gets changed on a watch event coming in...
// This is ueber-lame. But lack of threading + lack
// userspace consumers of this code pretty much condemns
// us to doing someting like this.
static enum { NOT_SIGNALLED, SIGNALLED } xenbus_watch_state = NOT_SIGNALLED;

// Inter-domain shared memory communications.
#define XENSTORE_RING_SIZE 1024
typedef uint32_t XENSTORE_RING_IDX;
#define MASK_XENSTORE_IDX(idx) ((idx) & (XENSTORE_RING_SIZE-1))

struct xenstore_domain_interface
{
    char req[XENSTORE_RING_SIZE]; /* Requests to xenstore daemon. */
    char rsp[XENSTORE_RING_SIZE]; /* Replies and async watch events. */
    XENSTORE_RING_IDX req_cons, req_prod;
    XENSTORE_RING_IDX rsp_cons, rsp_prod;
};

// xenbus interface.
static struct xenstore_domain_interface *xs_iface;

//______________________________________________________________________________
/// Copy string from ring with wraparound
//______________________________________________________________________________
static void
memcpy_from_ring(const void *ring,     ///< ring from which string is to be copied
		 void *dest,           ///< destination buffer
		 int off,              ///< starting offset into ring
		 int len               ///< number of bytes
		 )
{
    const char *r = ring;
    char *d = dest;
    int c1 = MIN(len, XENSTORE_RING_SIZE - off);
    int c2 = len - c1;
    memcpy(d, r + off, c1);
    memcpy(d + c1, r, c2);
}

//______________________________________________________________________________
/// Event channel handler.
//______________________________________________________________________________
static void
xenbus_event(evtchn_port_t port,           ///< port on which the event arrives
	     arch_interrupt_regs_t *regs,  ///< registers at time of event
	     void *ign                     ///< unused parameter
	     )
{
    xs_message_t msg;
    while (1)
	{
	    if (xs_iface->rsp_prod - xs_iface->rsp_cons < sizeof(msg))
		{
		    break;
		}
	    rmb();
	    memcpy_from_ring(xs_iface->rsp, &msg, MASK_XENSTORE_IDX(xs_iface->rsp_cons),
			     sizeof(msg));
	    if (xs_iface->rsp_prod - xs_iface->rsp_cons < sizeof(msg) + msg.len)
		{
		    break;
		}
	    if (msg.type == XS_WATCH_EVENT)
		{
		    xs_iface->rsp_cons += msg.len + sizeof(msg);
		    xenbus_watch_state = SIGNALLED;
		}
	    else
		{
		    xs_write_reply_t *reply_struct;
		    int status = handleGetReference(&reqIds, msg.req_id, (void**) &reply_struct);
		    BUG_ON(status != StatusOk);
		    reply_struct->reply = xalloc(charXtype, sizeof(msg) + msg.len);
		    BUG_ON(reply_struct->reply == NULL);
		    memcpy_from_ring(xs_iface->rsp,
				     reply_struct->reply, 
				     MASK_XENSTORE_IDX(xs_iface->rsp_cons),
				     msg.len + sizeof(msg));
		    xs_iface->rsp_cons += msg.len + sizeof(msg);
		    reply_struct->state = RECEIVED;
		}
	    wmb();
	}
}

//______________________________________________________________________________
/// Send data to xenbus. Blocks until done.
//______________________________________________________________________________
static void
xenbus_write_ring(
		  xsd_sockmsg_type_t type,         ///< one of the possible xenstore
		                                   ///< message types
		  int req_id,                      ///< request ID to match with return
		  xenbus_transaction_t trans_id,   ///< transaction number (or zero)
		  xs_write_req_t *req,             ///< array of the data to write
		  int nr_reqs                      ///< the size of the req array
		  )
{
    int r;
    int req_off;
    int total_off;
    int this_chunk;
    const xs_write_req_t *current_req;
    XENSTORE_RING_IDX prod;
    int len = 0;
    xs_message_t message = { .type = type, .req_id = req_id, .tx_id = trans_id };
    xs_write_req_t header = { &message, sizeof(message) };

    // Compute total length.
    for (r = 0; r < nr_reqs; r++)
	{
	    len += req[r].len;
	}
    message.len = len;
    len += sizeof(message);
    current_req = &header;

    // If the whole message cannot fit in one message, fail...
    BUG_ON(len > XENSTORE_RING_SIZE);

    // wait until there is room to write the whole message in one go
    prod = xs_iface->req_prod;
    if (prod + len - xs_iface->req_cons > XENSTORE_RING_SIZE) // don't need if? ---jas
	{
	    // Wait for space to become available.
	    while (xs_iface->req_prod + len - xs_iface->req_cons > XENSTORE_RING_SIZE)
		{
		    rmb();
		}
	    prod = xs_iface->req_prod;
	}

    // Should fit, now write each request in at most two chunks
    total_off = 0;
    req_off = 0;
    while (total_off < len)  // should use a memcpy_to_ring procedure internal to loop, with a for loop --jas
	{
	    this_chunk = MIN(current_req->len - req_off, XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(prod));
	    memcpy((char *) xs_iface->req + MASK_XENSTORE_IDX(prod),
		   (char *) current_req->data + req_off,
		   this_chunk);
	    prod += this_chunk;
	    req_off += this_chunk;
	    total_off += this_chunk;
	    if (req_off == current_req->len)
		{
		    req_off = 0;
		    if (current_req == &header)
			{
			    current_req = req;
			}
		    else
			{
			    current_req++;
			}
		}
	}

    BUG_ON(req_off != 0);
    BUG_ON(total_off != len);
    BUG_ON(prod > xs_iface->req_cons + XENSTORE_RING_SIZE);

    // force out ring buffer updates
    wmb();
    // Update indices.
    xs_iface->req_prod += len;

    // Notify remote.
    notify_remote_via_evtchn(start_info.store_evtchn);
}

//______________________________________________________________________________
/// Send a message to xenbus. Reply is malloced and is to be
/// freed by the caller.
//______________________________________________________________________________
static xs_message_t *
xenbus_msg_reply(
		 xsd_sockmsg_type_t type,          ///< message types
		 xenbus_transaction_t trans,       ///< transaction ID
		 xs_write_req_t *io,               ///< request to send
		 int nr_reqs                       ///< number of requests
		 )
{
    HandleId handleId;
    xs_write_reply_t reply = { .state = PENDING };
    int status = handleAllocate(&reqIds, &reply, &handleId);
    BUG_ON(status != StatusOk);
    ShortHandleId shortHandleId = handleId;
    xenbus_write_ring(type, shortHandleId, trans, io, nr_reqs);
    while (reply.state == PENDING)
	{
	    archKernelBlock();
	    rmb();
	}

    // Now we have a response.
    xs_message_t *rep = reply.reply;
    C_ASSERT(sizeof(shortHandleId) == sizeof(rep->req_id));

    BUG_ON(rep->req_id != shortHandleId);
    handleFree(&reqIds, shortHandleId);
    return rep;
}

//______________________________________________________________________________
/// Returns the ethos error code corresponding the Xen error passed.
//______________________________________________________________________________
static int
xenbus_error(
	     const char *xenErrorString     ///< xen string containing the error name
	     )
{
    int ethos_error;                 ///< return the ethos error number

    if (!strcmp(xenErrorString, "EINVAL"))
	{
	    ethos_error = StatusXenInvalidValue;
	}
    else if (!strcmp(xenErrorString, "ENOENT"))
	{
	    ethos_error = StatusNotFound;
	}
    else if (!strcmp(xenErrorString, "EACCES"))
	{
	    ethos_error = StatusNotAuthorized;
	}
    else if (!strcmp(xenErrorString, "EEXIST"))
	{
	    ethos_error = StatusExists;
	}
    else if (!strcmp(xenErrorString, "EISDIR"))
	{
	    ethos_error = StatusXenIsDirectory;
	}
    else if (!strcmp(xenErrorString, "ENOSPC"))
	{
	    ethos_error = StatusNoSpace;
	}
    else if (!strcmp(xenErrorString, "EIO"))
	{
	    ethos_error = StatusXenInputOutput;
	}
    else if (!strcmp(xenErrorString, "ENOTEMPTY"))
	{
	    ethos_error = StatusNotEmpty;
	}
    else if (!strcmp(xenErrorString, "ENOSYS"))
	{
	    ethos_error = StatusNotImplemented;
	}
    else if (!strcmp(xenErrorString, "EROFS"))
	{
	    ethos_error = StatusXenReadOnly;
	}
    else if (!strcmp(xenErrorString, "EBUSY"))
	{
	    ethos_error = StatusXenBusy;
	}
    else if (!strcmp(xenErrorString, "EAGAIN"))
	{
	    ethos_error = StatusXenAgain;
	}
    else if (!strcmp(xenErrorString, "EISCONN"))
	{
	    ethos_error = StatusXenComplete;
	}
    else  // still an error, fail
	{
	    ethos_error = StatusFail;
	}
    return ethos_error;
}

//______________________________________________________________________________
/// Lists values under a XenStore path.
/// Caller must free returned pointer with xfree.
//______________________________________________________________________________
int 
xenbus_ls(
	  xenbus_transaction_t xbt,  ///< transaction to get the directory info
	  const char *pre,           ///< the name in the directory tree
	  char ***contents           ///< return the conents of the directory subtree
	  )
{
    int nr_elems, x, i;
    char **res;
    xs_message_t *reply, *repmsg;
    int status = StatusOk;
    xs_write_req_t req[] = { { pre, strlen(pre) + 1 } };
    repmsg = xenbus_msg_reply(XS_DIRECTORY, xbt, req, ARRAY_SIZE(req));
    reply = repmsg + 1;
    if (repmsg->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (reply + 1));
	    goto done;
	}
    for (x = nr_elems = 0; x < repmsg->len; x++)
	{
	    nr_elems += (((char *)reply)[x] == 0);
	}
    res = xalloc(charXtype, sizeof(res[0]) * (nr_elems + 1));
    for (x = i = 0; i < nr_elems; i++)
	{
	    int l = strlen((char *)reply + x);
	    res[i] = xalloc(charXtype, l + 1);
	    memcpy(res[i], (char *)reply + x, l + 1);
	    x += l + 1;
	}
    res[i] = NULL;
    *contents = res;
 done:
    xfree(repmsg);
    return status;
}

//______________________________________________________________________________
/// Writes a value (must be a C string), to XenStore.
//______________________________________________________________________________
int 
xenbus_write(
	     xenbus_transaction_t xbt,      ///< transaction ID
	     const char *path,              ///< path in the directory
	     const char *value              ///< value to write
	     )
{
    int status = StatusOk;
    xs_write_req_t req[] =
	{
	    { path, strlen(path) + 1 },
	    // todo: changed from { value, strlen(value) + 1}
	    { value, strlen(value)}
	};
    xs_message_t *reply = xenbus_msg_reply(XS_WRITE, xbt, req, ARRAY_SIZE(req));
    if (reply->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (reply + 1));
	    goto done;
	}

 done:
    xfree(reply);
    return status;
}

//______________________________________________________________________________
/// Writes a formatted string to XenStore.
//______________________________________________________________________________
int 
xenbus_print(
	      xenbus_transaction_t xbt,      ///< transaction ID
	      const char *path,              ///< path in directory
	      const char *fmt,               ///< the format string
	      ...                            ///< the values to be formatted
	      )
{
    int status;
    va_list va;
    char *buffer = xalloc(charXtype, XENBUS_PRINTF_SIZE);
    if (!buffer)
	{
	    return StatusNoMemory;
	}
    va_start(va, fmt);
    vsnprint(buffer, XENBUS_PRINTF_SIZE, (char *)fmt, va);
    va_end(va);
    status = xenbus_write(xbt, path, buffer);
    xfree(buffer);
    return status;
}

//______________________________________________________________________________
/// Remove a node specified by path.
//______________________________________________________________________________
int
xenbus_rm(
	  xenbus_transaction_t xbt,           ///< transaction ID
	  const char *path                    ///< path in directory
	  )
{
    int status = StatusOk;
    xs_write_req_t req[] = { { path, strlen(path) + 1 } };
    xs_message_t *reply = xenbus_msg_reply(XS_RM, xbt, req, ARRAY_SIZE(req));
    if (reply->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (reply + 1));
	    goto done;
	}
 done:
    xfree(reply);
    return status;
}

//______________________________________________________________________________
/// Start a XenStore transaction.
//______________________________________________________________________________
int
xenbus_begin(xenbus_transaction_t *xbt           ///< transaction ID return
	     )
{
    int status = StatusOk;
    xs_write_req_t req = { "", 1 };

    xs_message_t *reply = xenbus_msg_reply(XS_TRANSACTION_START, XBT_NIL, &req, 1);
    if (reply->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (reply + 1));
	    goto done;
	}
    sscanf((char*) (reply + 1), "%u", xbt);

 done:
    xfree(reply);
    return status;
}

//______________________________________________________________________________
/// Ends a transaction (aborts in abort is TRUE).
//______________________________________________________________________________
int
xenbus_end(xenbus_transaction_t xbt,           ///< transaction ID
	   bool abort                          ///< if true, abort the transaction
	   ) 
{
    int status = StatusOk;
    xs_write_req_t req = { abort ? "F" : "T", 2 };
    xs_message_t *reply = xenbus_msg_reply(XS_TRANSACTION_END, xbt, &req, 1);
    if (reply->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (reply + 1));
	    goto done;
	}

 done:
    xfree(reply);
    return status;
}

//______________________________________________________________________________
/// Get permissions on a path node.
/// Value must be freed by xfree by caller.
//______________________________________________________________________________
int
xenbus_get_perms(
		 xenbus_transaction_t xbt,        ///< transaction ID
		 const char *path,                ///< path in directory tree
		 char **value                     /// permission found there
		 )
{
    char *res;
    int status = StatusOk;
    xs_write_req_t req[] = { { path, strlen(path) + 1 } };
    xs_message_t *reply = xenbus_msg_reply(XS_GET_PERMS, xbt, req, ARRAY_SIZE(req));
    if (reply->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (reply + 1));
	    goto done;
	}
    res = xalloc(charXtype, reply->len + 1);
    memcpy(res, reply + 1, reply->len);
    res[reply->len] = 0;
    *value = res;

 done:
    xfree(reply);
    return status;
}

static
int
_self_domid(int *domid_ptr)
{
    char* domid_str = NULL;
    int i, domid = 0;

    Status status = xenbus_read(XBT_NIL, "domid", &domid_str);
    if (StatusOk != status)
	{
	    goto done;
	}

    // Yuck we need an atoi
    for (i = 0; domid_str[i] != '\0'; ++i)
	{
	    domid = domid * 10 + domid_str[i] - '0';
	}

    xfree(domid_str);
    *domid_ptr = domid;

 done:
    return status;
}

#define PERM_MAX_SIZE 32
//______________________________________________________________________________
/// Sets permissions on a path node.
//______________________________________________________________________________
int
xenbus_set_perms(
		 xenbus_transaction_t xbt,       ///< transaction ID
		 const char *path,               ///< path in XenStore
		 domid_t domain,                 ///< domain the permissions apply to
		 char perm                       ///< the permissions
		 )
{
    char owner[PERM_MAX_SIZE];
    char foreign[PERM_MAX_SIZE];
    int status;

    int self = -1;
    status = _self_domid(&self);
    REQUIRE_OK(status);

    // First element is the owner, permissions for domains not listed = none.
    snprint(owner, PERM_MAX_SIZE, "%c%hu", XENBUS_PERM_NONE, self);
    // Listed domains and their permissions. Could be more than one.
    snprint(foreign, PERM_MAX_SIZE, "%c%hu", perm, domain);

    // set_perms protocol: path, owner, other domains
    xs_write_req_t req[] =
	{
	    { path, strlen(path) + 1 },
	    { owner, strlen(owner) + 1 },
	    { foreign, strlen(foreign) + 1 }
	};
    xs_message_t *reply = xenbus_msg_reply(XS_SET_PERMS, xbt, req, ARRAY_SIZE(req));
    if (reply->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (reply + 1));
	    goto done;
	}
 done:
    xfree(reply);
    return status;
}

//______________________________________________________________________________
/// Reads a value from XenStore.
/// Caller must free returned pointer with xfree iff status equals StatusOk.
//______________________________________________________________________________
int 
xenbus_read(
	    xenbus_transaction_t xbt,             ///< transaction ID
	    const char *path,                     ///< path in XenStore
	    char **value                          ///< value to be returned
	    )
{
    char *res;
    xs_message_t *repmsg;
    int status = StatusOk;
    xs_write_req_t req[] = { { path, strlen(path) + 1 } };
    repmsg = xenbus_msg_reply(XS_READ, xbt, req, ARRAY_SIZE(req));
    if (repmsg->type == XS_ERROR)
	{
	    // status not StatusOk
	    status = xenbus_error((char*) (repmsg + 1));
	    goto done;
	}
    res = xalloc(charXtype, repmsg->len + 1);
    memcpy(res, repmsg + 1, repmsg->len);
    res[repmsg->len] = 0;
    *value = res;
 done:
    xfree(repmsg);
    return status;
}

//______________________________________________________________________________
/// Watch a xenbus path.
/// Returns when value of path matches value.
//______________________________________________________________________________
int
xenbus_watch(
	     xenbus_transaction_t xbt,            ///< transaction ID
	     const char *path                     ///< path in XenStore
	     )
{
    xs_message_t *rep;
    xs_write_req_t req[] =
	{
	    { path, strlen(path) + 1 },
	    { "0", 2 }
	};
    int status = StatusOk;
    rep = xenbus_msg_reply(XS_WATCH, xbt, req, ARRAY_SIZE(req));
    if (rep->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (rep + 1));
	    goto done;
	}
    for(;;)
	{
	    char *output = NULL;

	    // Sit waiting for the watch to come in...
	    while (xenbus_watch_state == NOT_SIGNALLED)
		{
		    archKernelBlock();
		    rmb();
		}
	    xenbus_watch_state = NOT_SIGNALLED;

	    // Read and compare values.
	    status = xenbus_read(XBT_NIL, path, &output);
	    if (status == StatusOk)
		{
		    xfree(output);
		    break;
		}
	}

 done:
    xfree(rep);
    return status;
}

//______________________________________________________________________________
/// Watch a xenbus path.
/// Returns when value of path matches value.
//______________________________________________________________________________
int
xenbus_watch_value(
		   xenbus_transaction_t xbt,    ///< transaction ID
		   const char *path,            ///< path to watch
		   const char *value            ///< value on which to return
		   )
{
    xs_message_t *rep;
    xs_write_req_t req[] =
	{
	    { path, strlen(path) + 1 },
	    { "0", 2 }
	};
    int status = StatusOk;
    rep = xenbus_msg_reply(XS_WATCH, xbt, req, ARRAY_SIZE(req));
    if (rep->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (rep + 1));
	    goto done;
	}
    for (;;)
	{
	    int c;
	    char *output = NULL;

	    // Sit waiting for the watch to come in...
	    while (xenbus_watch_state == NOT_SIGNALLED)
		{
		    archKernelBlock();
		    rmb();
		}
	    xenbus_watch_state = NOT_SIGNALLED;

	    // Read and compare values.
	    status = xenbus_read(XBT_NIL, path, &output);
	    if (status == StatusOk)
		{
		    c = strcmp(output, value);
		    xfree(output);
		    if (!c)
			{
			    // Done;
			    break;
			}
		}
	}

    // Once we're done, destroy the watch
    rep = xenbus_msg_reply(XS_UNWATCH, xbt, req, ARRAY_SIZE(req));
    if (rep->type == XS_ERROR)
	{
	    status = xenbus_error((char*) (rep + 1));
	    goto done;
	}

 done:
    xfree(rep);
    return status;
}

//______________________________________________________________________________
/// Initializes xenbus code.
//______________________________________________________________________________
void 
xenbus_init(void)
{
    int rc = handleTableInit(&reqIds);
    BUG_ON(rc != StatusOk);
    xs_iface = (void*) mfnToVirtual(start_info.store_mfn);

    // Register the event handler for xenstore.
    int evtchan = xenEventBind(start_info.store_evtchn, &xenbus_event, NULL);
    BUG_ON(evtchan <= 0);
    printfLog("XenStore channel on 0x%x, evtchn  0x%x\n", xs_iface, evtchan);
}

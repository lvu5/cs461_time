//______________________________________________________________________________
// Jun-2008: Andrei Warkentin
// Xenbus/XenStore code.
//______________________________________________________________________________

#ifndef __XENBUS_H__
#define __XENBUS_H__

typedef unsigned long xenbus_transaction_t;
#define XBT_NIL ((xenbus_transaction_t) 0)

#define XENBUS_PERM_READ       'r'
#define XENBUS_PERM_WRITE      'w'
#define XENBUS_PERM_READ_WRITE 'b'	// "both"
#define XENBUS_PERM_NONE       'n'	// "no access"

// Initializes xenbus/xenstore.
void xenbus_init(void);

// Writes a formatted string to XenStore.
int xenbus_print(
		  xenbus_transaction_t xbt,
		  const char *path,
		  const char *fmt,
		  ...
		  );

// Watch a xenbus path.
// Returns when value of path matches value.
int xenbus_watch_value(
		       xenbus_transaction_t xbt,
		       const char *path,
		       const char *value
		       );

// Watch a xenbus path.
// Returns when path node becomes available.
int xenbus_watch(
		 xenbus_transaction_t xbt,
		 const char *path
		 );

// Lists values under a XenStore path.
// Caller must free returned pointer with kfree.
int xenbus_ls(
	      xenbus_transaction_t xbt,
	      const char *pre,
	      char ***contents
	      );

// Writes a value (must be a C string), to XenStore.
int xenbus_write(
		 xenbus_transaction_t xbt,
		 const char *path,
		 const char *value
		 );

// Remove a node specified by path.
int xenbus_rm(
	      xenbus_transaction_t xbt,
	      const char *path
	      );

// Start a XenStore transaction.
int xenbus_begin(xenbus_transaction_t *xbt);

// Ends a transaction (aborts in abort is true).
int xenbus_end(xenbus_transaction_t xbt, bool abort);

// Get permissions on a path node.
// Value must be freed by kfree by caller.
int xenbus_get_perms(
		     xenbus_transaction_t xbt,
		     const char *path,
		     char **value
		     );

// Sets permissions on a path node.
int xenbus_set_perms(
		     xenbus_transaction_t xbt, 
		     const char *path,
		     domid_t domain,
		     char perm
		     );

// Reads a value from XenStore.
// Caller must free returned pointer with kfree.
int xenbus_read(
		xenbus_transaction_t xbt, 
		const char *path, 
		char **value
		);

#endif

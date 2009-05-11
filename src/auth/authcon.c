/*
 * Copyright 2000, International Business Machines Corporation and others.
 * All Rights Reserved.
 * 
 * This software has been released under the terms of the IBM Public
 * License.  For details, see the LICENSE file in the top-level source
 * directory or online at http://www.openafs.org/dl/license10.html
 */

#include <afsconfig.h>
#if defined(UKERNEL)
#include "afs/param.h"
#else
#include <afs/param.h>
#endif

RCSID
    ("$Header$");


#if defined(UKERNEL)
#include "afs/cellconfig.h"
#ifdef AFS_RXK5
/* BEWARE: this code uses "u".  Must include heimdal krb5.h (u field name)
 * before libuafs afs/sysincludes.h (libuafs makes u a function.)
 */
#include <rxk5_utilafs.h>
#include <rx/rxk5.h>
#endif
#include "afs/sysincludes.h"
#include "afsincludes.h"
#include "afs/stds.h"
#include "afs/pthread_glock.h"
#include "des/des.h"
#include "des/des_prototypes.h"
#include "rx/rxkad.h"
#include "rx/rx.h"
#include "afs/keys.h"
#include "afs/auth.h"
#include "afs/pthread_glock.h"
#else /* defined(UKERNEL) */
#include <afs/stds.h>
#include <afs/pthread_glock.h>
#include <sys/types.h>
#ifdef AFS_NT40_ENV
#include <winsock2.h>
#else
#include <sys/file.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#if defined(AFS_RXK5) && !defined(AFS_NT40_ENV)
#include <string.h>
#include <stdio.h>
#include <des.h>
#endif
#if !defined(AFS_RXK5) || !defined(AFS_NT40_ENV)
#			include <stdio.h>	// XXX why?
#			include <des/des.h>	// XXX why?
#include <des_prototypes.h>
#endif
#include <rx/rxkad.h>
#include <rx/rx.h>
#include "cellconfig.h"
#include "keys.h"
#include "auth.h"
#ifdef AFS_RXK5
#include <rxk5_utilafs.h>
#include <rx/rxk5.h>
#endif
#endif /* defined(UKERNEL) */
#include <errno.h>

/* return a null security object if nothing else can be done */
static afs_int32
QuickAuth(struct rx_securityClass **astr, afs_int32 *aindex)
{
    register struct rx_securityClass *tc;
    tc = rxnull_NewClientSecurityObject();
    *astr = tc;
    *aindex = 0;
    return 0;
}

#if !defined(UKERNEL)
/* Return an appropriate set of security classes and indexes */
	/* this is mainly for use by ubik servers */

/* XXX 1.5.59 has "*aindex" not "maxindex".  Why? */
afs_int32
afsconf_ServerAuth(void *arock,
		   struct rx_securityClass **astr, 
		   afs_int32 maxindex)
{
    int i, r;
    struct afsconf_dir *adir = arock;

    LOCK_GLOBAL_MUTEX;
    r = 0;
    if (maxindex
	    && (astr[0] = rxnull_NewServerSecurityObject())) {
	if (!r) r = 1;
    }
#ifdef AFS_RXK5
    if (maxindex > 5
	    && have_afs_rxk5_keytab(adir->name)
	    && (astr[5] = rxk5_NewServerSecurityObject(rxk5_auth,
		get_afs_rxk5_keytab(adir->name),
		rxk5_default_get_key, 0, 0))) {
	if (r < 6) r = 6;
    } else
#endif
    if (maxindex > 2
#ifdef AFS_RXK5
	    && have_afs_keyfile(adir)
#endif
	    && (astr[2] = rxkad_NewServerSecurityObject(0, (char *) adir,
		afsconf_GetKey, NULL))) {
	if (r < 3) r = 3;
    }
    UNLOCK_GLOBAL_MUTEX;
    return r;
}
#endif /* !defined(UKERNEL) */

static afs_int32
GenericAuth(struct afsconf_dir *adir,
    struct rx_securityClass **astr,
    afs_int32 *aindex,
    afs_int32 flags)
{
    char tbuffer[256];
    struct ktc_encryptionKey key, session;
    struct rx_securityClass *tclass;
    afs_int32 kvno;
    afs_int32 ticketLen;
    register afs_int32 code;
    rxkad_level enclevel;
#ifdef AFS_RXK5
    krb5_creds *k5_creds, in_creds[1];
    krb5_context k5context;
#endif

    enclevel = (flags & FORCE_SECURE) ? rxkad_crypt : rxkad_clear;

    if (!(flags & (FORCE_K5CC|FORCE_KTC)))
	flags |= (FORCE_K5CC|FORCE_KTC);
    
#ifdef AFS_RXK5
    
    if((flags & FORCE_K5CC) && have_afs_rxk5_keytab(adir->name)) {

	k5context = rxk5_get_context(0);
		
	/* forge credentials using the k5 key of afs */
	memset(in_creds, 0, sizeof *in_creds);
	code = default_afs_rxk5_forge(k5context, adir, 0, in_creds);
	if(code) {
	    return code;
	}
	k5_creds = in_creds;
	/* enclevel could be 0 or 2.  set output to be auth or crypt. */
	tclass = rxk5_NewClientSecurityObject(rxk5_auth + (enclevel==rxkad_crypt),
		k5_creds, 0);
	
	*astr = tclass;
        *aindex = 5;	
	goto out;
    }
    
#endif

    /* first, find the right key and kvno to use */
    if ((flags & FORCE_KTC)) {
	code = afsconf_GetLatestKey(adir, &kvno, &key);
	if (code && (flags & FORCE_FAILAUTH))
	    return code;
    } else code = EDOM;
    if (code) {
	return QuickAuth(astr, aindex);
    }

    /* next create random session key, using key for seed to good random */
    des_init_random_number_generator(key.data);
    code = des_random_key(session.data);
    if (code) {
	return QuickAuth(astr, aindex);
    }

    /* now create the actual ticket */
    ticketLen = sizeof(tbuffer);
    memset(tbuffer, '\0', sizeof(tbuffer));
    code =
	tkt_MakeTicket(tbuffer, &ticketLen, &key, AUTH_SUPERUSER, "", "", 0,
		       0xffffffff, &session, 0, "afs", "");
    /* parms were buffer, ticketlen, key to seal ticket with, principal
     * name, instance and cell, start time, end time, session key to seal
     * in ticket, inet host, server name and server instance */
    if (code) {
	return QuickAuth(astr, aindex);
    }

    /* Next, we have ticket, kvno and session key, authenticate the connection.
     * We use a magic # instead of a constant because of basic compilation
     * order when compiling the system from scratch (rx/rxkad.h isn't installed
     * yet). */
    tclass = (struct rx_securityClass *)
	rxkad_NewClientSecurityObject(enclevel, &session, kvno, ticketLen,
				      tbuffer);
    *astr = tclass;
    *aindex = 2;		/* kerberos security index */
 
out:   
    return 0;
}

/* build a fake ticket for 'afs' using keys from adir, returning an
 * appropriate security class and index
 */
afs_int32
afsconf_ClientAuth(void *arock, struct rx_securityClass ** astr,
		   afs_int32 * aindex)
{
    struct afsconf_dir * adir = (struct afsconf_dir *) arock;
    afs_int32 rc;

    LOCK_GLOBAL_MUTEX;
    rc = GenericAuth(adir, astr, aindex, 0);
    UNLOCK_GLOBAL_MUTEX;
    return rc;
}

/* build a fake ticket for 'afs' using keys from adir, returning an
 * appropriate security class and index.  This one, unlike the above,
 * tells rxkad to encrypt the data, too.
 */
afs_int32
afsconf_ClientAuthSecure(struct afsconf_dir *adir,
			 struct rx_securityClass **astr, 
			 afs_int32 *aindex)
{
    afs_int32 rc;

    LOCK_GLOBAL_MUTEX;
    rc = GenericAuth(adir, astr, aindex, FORCE_SECURE);
    UNLOCK_GLOBAL_MUTEX;
    return rc;
}

/* build a fake ticket for 'afs' using keys from adir, returning an
 * appropriate security class and index.  This one, unlike the above,
 * tells rxkad to encrypt the data, too.
 */
afs_int32
afsconf_ClientAuthEx(struct afsconf_dir *adir,
     struct rx_securityClass **astr,
     afs_int32 *aindex,
     afs_int32 flags)
{
    afs_int32 rc;

    LOCK_GLOBAL_MUTEX;
    rc = GenericAuth(adir, astr, aindex, flags);
    UNLOCK_GLOBAL_MUTEX;
    return rc;
}

/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2014 MonetDB B.V.
 * All Rights Reserved.
 */

#ifndef _GROUP_H_
#define _GROUP_H_
#include "gdk.h"

#ifdef WIN32
#if !defined(LIBMAL) && !defined(LIBATOMS) && !defined(LIBKERNEL) && !defined(LIBMAL) && !defined(LIBOPTIMIZER) && !defined(LIBSCHEDULER) && !defined(LIBMONETDB5)
#define group_export extern __declspec(dllimport)
#else
#define group_export extern __declspec(dllexport)
#endif
#else
#define group_export extern
#endif

group_export str GRPsubgroup1(bat *ngid, bat *next, bat *nhis,
							  const bat *bid);
group_export str GRPsubgroup2(bat *ngid, bat *next, bat *nhis,
							  const bat *bid, const bat *gid);
group_export str GRPsubgroup4(bat *ngid, bat *next, bat *nhis,
							  const bat *bid, const bat *gid,
							  const bat *eid, const bat *hid);

#endif /* _GROUP_H_ */

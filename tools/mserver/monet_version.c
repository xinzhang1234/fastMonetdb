/* -*-C-*- */

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2017 MonetDB B.V.
 */

#include "monetdb_config.h"
#include "mal.h"
#include <stdio.h>
#include "monet_version.h"
#ifdef HAVE_LIBPCRE
#include <pcre.h>
#endif
#ifdef HAVE_OPENSSL
#include <openssl/opensslv.h>
#endif
#ifdef HAVE_LIBXML
#include <libxml/xmlversion.h>
#endif

void
monet_version(void)
{
	dbl sz_mem_gb;
	int cores;

	MT_init();  /* for MT_pagesize */
	sz_mem_gb = (dbl)(MT_npages() * MT_pagesize()) / (1024.0 * 1024.0 * 1024.0);
	cores = MT_check_nr_cores();

	printf("MonetDB 5 server v" VERSION " ");
	/* coverity[pointless_string_compare] */
	if (strcmp(MONETDB_RELEASE, "unreleased") != 0)
		printf("\"%s\" ", MONETDB_RELEASE);
	printf("(" SZFMT "-bit%s)\n",
			(size_t) (sizeof(ptr) * 8),
#ifdef HAVE_HGE
			", 128-bit integers"
#else
			""
#endif
	);
	/* coverity[pointless_string_compare] */
	if (strcmp(MONETDB_RELEASE, "unreleased") == 0)
		printf("This is an unreleased version\n");
	printf("Copyright (c) 1993-July 2008 CWI\n"
			"Copyright (c) August 2008-2017 MonetDB B.V., all rights reserved\n");
	printf("Visit https://www.monetdb.org/ for further information\n");
	printf("Found %.1fGiB available memory, %d available cpu core%s\n",
			sz_mem_gb, cores, cores != 1 ? "s" : "");
	/* don't want to GDKinit just for this
			"using %d thread%s\n",
			GDKnr_threads, GDKnr_threads != 1 ? "s" : ""); */
	printf("Libraries:\n");
#ifdef HAVE_LIBPCRE
	{
		char pcreversion[] = "compiled with 8.31";
		printf("  libpcre: %s", pcre_version());
		if (*pcreversion)
			printf(" (%s)", pcreversion);
		printf("\n");
	}
#endif
#ifdef HAVE_OPENSSL
	{
		char opensslversion[] = "compiled with OpenSSL 1.0.1f 6 Jan 2014";
		printf("  openssl: %s", OPENSSL_VERSION_TEXT);
		if (*opensslversion)
			printf(" (%s)", opensslversion);
		printf("\n");
	}
#endif
#ifdef HAVE_LIBXML
	{
		char libxml2version[] = "compiled with 2.9.1";
		printf("  libxml2: %s", LIBXML_DOTTED_VERSION);
		if (*libxml2version)
			printf(" (%s)", libxml2version);
		printf("\n");
	}
#endif
	printf("Compiled by: %s (" HOST ")\n", "xzhang3@xia-wimpy");
	printf("Compilation: %s\n", "gcc  -g  -Werror -Wall -Wextra -W -Werror-implicit-function-declaration -Wpointer-arith -Wdeclaration-after-statement -Wundef -Wformat=2 -Wno-format-nonliteral -Winit-self -Winvalid-pch -Wmissing-declarations -Wmissing-format-attribute -Wmissing-prototypes -Wold-style-definition -Wpacked -Wunknown-pragmas -Wvariadic-macros -fstack-protector-all -Wstack-protector -Wpacked-bitfield-compat -Wsync-nand -Wjump-misses-init -Wmissing-include-dirs -Wlogical-op -Wnested-externs -Wunreachable-code");
	printf("Linking    : %s\n", "/usr/bin/ld -m elf_x86_64 ");
}

# $NetBSD: Makefile.inc,v 1.2 2017/06/12 14:50:38 roy Exp $

SRCDIR=		${NETBSDSRCDIR}/external/bsd/dhcpcd
DIST=		${SRCDIR}/dist

SED_SYS=	-e 's:@SYSCONFDIR@:/etc:g' \
		-e 's:@DATADIR@:/usr/share/examples:g' \
		-e 's:@DBDIR@:/var/db:g' \
		-e 's:@LIBDIR@:/lib:g' \
		-e 's:@RUNDIR@:/var/run:g' \
		-e 's:@HOOKDIR@:/libexec/dhcpcd-hooks:g' \
		-e 's:@SCRIPT@:/libexec/dhcpcd-run-hooks:g' \
		-e 's:@SERVICEEXISTS@::g' \
		-e 's:@SERVICECMD@::g' \
		-e 's:@SERVICESTATUS@::g' \
		-e 's:@STATUSARG@::g'

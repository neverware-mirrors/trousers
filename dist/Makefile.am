EXTRA_DIST = system.data.auth system.data.noauth \
       fedora/fedora.initrd.tcsd

install: install-exec-hook
	if test ! -e ${DESTDIR}/@sysconfdir@/tcsd.conf; then mkdir -p ${DESTDIR}/@sysconfdir@ && cp tcsd.conf ${DESTDIR}/@sysconfdir@; fi
if !NOUSERCHECK
	/bin/chown root:tss ${DESTDIR}/@sysconfdir@/tcsd.conf || true
	/bin/chmod 0640 ${DESTDIR}/@sysconfdir@/tcsd.conf
endif

install-exec-hook:
	/bin/sh -c 'if [ ! -e ${DESTDIR}/@localstatedir@/lib/tpm ];then mkdir -p ${DESTDIR}/@localstatedir@/lib/tpm; fi'
if !NOUSERCHECK
	/usr/sbin/groupadd tss || true
	/usr/sbin/useradd -r tss -g tss || true
	/bin/chown tss:tss ${DESTDIR}/@localstatedir@/lib/tpm || true
	/bin/chmod 0700 ${DESTDIR}/@localstatedir@/lib/tpm
endif

uninstall-hook:
	rm ${DESTDIR}/@sysconfdir@/tcsd.conf
	rmdir ${DESTDIR}/@localstatedir@/lib/tpm
if !NOUSERCHECK
	/usr/sbin/userdel tss || true
	/usr/sbin/groupdel tss || true
endif

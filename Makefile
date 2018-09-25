mode = debug

ARCH ?= $(shell dpkg-architecture -qDEB_HOST_MULTIARCH)
LIB_PATH = /usr/lib/$(ARCH)
PATH_ARGS = UKUI_BIOMETRIC=/usr/share/ukui-biometric INSTALL_ROOT=$(DESTDIR)/
UKUI_BIOMETRIC=/usr/share/ukui-biometric

# Target
all:
	cp polkit-agent/data/polkit-ukui-authentication-agent-1.desktop.in \
		polkit-agent/data/polkit-ukui-authentication-agent-1.desktop
	sed -i 's/@LIB_PATH/\/usr\/lib\/$(ARCH)/g' \
		polkit-agent/data/polkit-ukui-authentication-agent-1.desktop
	$(MAKE) -C pam-biometric/ $(PATH_ARGS)
	qmake bioauth/ $(PATH_ARGS) -o bioauth/Makefile
	$(MAKE) -C bioauth/ $(PATH_ARGS)
	qmake bioauth-bin/ $(PATH_ARGS) -o bioauth-bin/Makefile
	$(MAKE) -C bioauth-bin/ $(PATH_ARGS)
	qmake polkit-agent/ $(PATH_ARGS) -o polkit-agent/Makefile
	$(MAKE) -C polkit-agent/ $(PATH_ARGS)

install: install-pam install-bin install-images install-polkit

install-pam:
	# install pam-biometric
	$(MAKE) -C pam-biometric install $(PATH_ARGS)

install-bin:
	# install bioauth-bin
	$(MAKE) -C bioauth-bin install $(PATH_ARGS)

install-images:
	# install images
	mkdir -p $(DESTDIR)$(UKUI_BIOMETRIC)/images
	install -m 644 -D images/* $(DESTDIR)$(UKUI_BIOMETRIC)/images/

install-polkit:
	# install polkit-ukui-agent
	$(MAKE) -C polkit-agent install $(PATH_ARGS) LIB_PATH=$(LIB_PATH)

uninstall: uninstall-pam uninstall-bin uninstall-images uninstall-polkit

uninstall-pam:
	# uninstall pam-biometric
	$(MAKE) -c pam-biometric uninstall $(PATH_ARGS)

uninstall-bin:
	# uninstall bioauth-bin
	$(MAKE) -C bioauth-bin uninstall $(PATH_ARGS)

uninstall-images:
	# uninstall images
	rm -rf $(DESTDIR)$(UKUI_BIOMETRIC)/images

uninstall-polkit:
	# uninstall polkit-ukui-agent
	$(MAKE) -C polkit-agent uninstall $(PATH_ARGS) LIB_PATH=$(LIB_PATH)

clean:
	# Clean
	$(MAKE) -C bioauth clean
	$(MAKE) -C bioauth-bin clean
	$(MAKE) -C pam-biometric clean
	$(MAKE) -C polkit-agent clean
	rm -f bioauth/i18n_ts/*.qm
	rm -f bioauth-bin/i18n_ts/*.qm
	rm -f polkit-agent/i18n_ts/*.qm
	rm -f polkit-agent/data/polkit-ukui-authentication-agent-1.desktop

distclean:
	# dist clean
	$(MAKE) -C bioauth distclean
	$(MAKE) -C bioauth-bin distclean
	$(MAKE) -C pam-biometric distclean
	$(MAKE) -C polkit-agent distclean

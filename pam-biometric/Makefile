CC = gcc
CFLAGS = -Wall -fPIC
INCLUDE = -I../common
PAM_LDFLAGS = -lpam
PAM_MODULE_DIR = /lib/security
PAM_CONFIGS = /usr/share/pam-configs
OBJS = pam_biometric.o logger.o
UKUI_BIOMETRIC=/usr/share/ukui-biometric
CONFIG_FILE=$(UKUI_BIOMETRIC)/biometric-auth.conf
POLKIT_ACTION_DIR=/usr/share/polkit-1/actions
ifeq ($(mode), debug)
	CFLAGS += -g
else
	CFLAGS += -O2
endif

all: pam_biometric.so

pam_biometric.so: $(OBJS)
	$(CC) $(CFLAGS) -shared $^ -o $@ $(INCLUDE) $(PAM_LDFLAGS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -DUKUI_BIOMETRIC=$(UKUI_BIOMETRIC) -DCONFIG_FILE=$(CONFIG_FILE) -c -o $@ $< $(INCLUDE)

install:
	install -D pam_biometric.so $(DESTDIR_PAM)$(PAM_MODULE_DIR)/pam_biometric.so
	install -m 644 -D pam-configs/pam-biometric $(DESTDIR_PAM)$(PAM_CONFIGS)/pam-biometric
	install -m 644 -D data/biometric-auth.conf $(DESTDIR_PAM)$(UKUI_BIOMETRIC)/biometric-auth.conf
	install -m 644 -D data/org.freedesktop.plicykit.pkexec.bioctl.policy \
		$(DESTDIR_PAM)$(POLKIT_ACTION_DIR)/org.freedesktop.plicykit.pkexec.bioctl.policy
	install -m 755 -D utils/bioctl $(DESTDIR_PAM)/bin/bioctl

uninstall:
	rm -f $(DESTDIR_PAM)$(PAM_MODULE_DIR)/pam_biometric.so
	rm -f $(DESTDIR_PAM)$(PAM_CONFIGS)/pam-biometric
	rm -f $(DESTDIR_PAM)$(UKUI_BIOMETRIC)/biometric-auth.conf
	rm -f $(DESTDIR_PAM)/bin/bioctl
	rm -f $(DESTDIR_PAM)$(POLKIT_ACTION_DIR)/org.freedesktop.plicykit.pkexec.bioctl.policy

clean:
	rm -f *.o pam_biometric.so

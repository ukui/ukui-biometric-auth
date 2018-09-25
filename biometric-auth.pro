TEMPLATE = subdirs

SUBDIRS += \
        bioauth \
        pam-biometric \
        pam-biometric-dialog \
        polkit-agent

CONFIG += ordered debug

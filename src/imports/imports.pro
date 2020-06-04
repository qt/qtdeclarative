TEMPLATE = subdirs
SUBDIRS += \
    controls \
    platform \
    templates \
    nativestyle

SUBDIRS += \
    controls/fusion/fusion.pro \
    controls/imagine/imagine.pro \
    controls/material/material.pro \
    controls/universal/universal.pro

macos: SUBDIRS += controls/macos/macos.pro
win32: SUBDIRS += controls/windows/windows.pro
unix: SUBDIRS += controls/fusiondesktop/fusiondesktop.pro


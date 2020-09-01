TEMPLATE = subdirs
SUBDIRS += \
    controls \
    controlsimpl \
    platform \
    templates \
    nativestyle

SUBDIRS += \
    controls/basic/basic.pro \
    controls/basic/impl/basic-impl.pro \
    controls/fusion/fusion.pro \
    controls/fusion/impl/fusion-impl.pro \
    controls/imagine/imagine.pro \
    controls/imagine/impl/imagine-impl.pro \
    controls/material/material.pro \
    controls/material/impl/material-impl.pro \
    controls/universal/universal.pro \
    controls/universal/impl/universal-impl.pro

macos: SUBDIRS += controls/macos/macos.pro
win32: SUBDIRS += controls/windows/windows.pro

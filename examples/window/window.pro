TEMPLATE = subdirs
SUBDIRS += \
    #screen \
    window

# install
sources.files = window.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/window
INSTALLS += sources

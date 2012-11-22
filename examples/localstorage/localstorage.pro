TEMPLATE = subdirs
SUBDIRS += localstorage

# install
sources.files = localstorage.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/localstorage
INSTALLS += sources

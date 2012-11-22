TEMPLATE = subdirs
SUBDIRS += gettingStartedQml

# install
sources.files = tutorials.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/tutorials
INSTALLS += sources

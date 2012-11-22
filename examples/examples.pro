TEMPLATE = subdirs
SUBDIRS += \
    demos \
    shared \
    localstorage \
    particles \
    qml \
    quick \
    tutorials \
    window \
    qmltest

# install
sources.files = shared examples.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/
INSTALLS += sources

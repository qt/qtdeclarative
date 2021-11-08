TEMPLATE = subdirs
SUBDIRS += affectors \
    emitters \
    imageparticle \
    itemparticle \
    system

#Install shared images too
qml.files = images
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/particles
INSTALLS += qml

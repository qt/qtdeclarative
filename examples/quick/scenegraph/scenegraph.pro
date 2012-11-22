TEMPLATE = subdirs
SUBDIRS += customgeometry simplematerial openglunderqml

# install
sources.files = scenegraph.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/scenegraph
INSTALLS += sources

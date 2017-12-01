TEMPLATE = subdirs
QT_FOR_CONFIG += qml

!emacsripten:qtConfig(qml-debug):SUBDIRS += qmltooling
qtHaveModule(quick):SUBDIRS += scenegraph

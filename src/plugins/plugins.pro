TEMPLATE = subdirs
QT_FOR_CONFIG += qml

qtConfig(qml-debug):SUBDIRS += qmltooling
qtHaveModule(quick):SUBDIRS += scenegraph

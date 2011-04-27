TEMPLATE = lib
CONFIG += qt plugin
QT += declarative

DESTDIR = QWidgets
TARGET = qmlqwidgetsplugin

SOURCES += qwidgets.cpp

sources.files += qwidgets.pro qwidgets.cpp qwidgets.qml
sources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/plugins
target.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/plugins

INSTALLS += sources target

symbian:{
    CONFIG += qt_example
    TARGET.EPOCALLOWDLLDATA = 1

    importFiles.files = QWidgets/qmlqwidgetsplugin.dll QWidgets/qmldir
    importFiles.path = QWidgets

    DEPLOYMENT += importFiles
}

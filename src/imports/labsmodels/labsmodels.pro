CXX_MODULE = qml
TARGET  = labsmodelsplugin
TARGETPATH = Qt/labs/qmlmodels
QML_IMPORT_VERSION = $$QT_VERSION

QT = qml-private qmlmodels-private

SOURCES += \
    plugin.cpp

qtConfig(qml-table-model) {
    SOURCES += \
        $$PWD/qqmltablemodel.cpp \
        $$PWD/qqmltablemodelcolumn.cpp

    HEADERS += \
        $$PWD/qqmltablemodel_p.h \
        $$PWD/qqmltablemodelcolumn_p.h
}

qtConfig(qml-delegate-model) {
    SOURCES += \
        qqmldelegatecomponent.cpp

    HEADERS += \
        qqmldelegatecomponent_p.h
}

CONFIG += qmltypes install_qmltypes
load(qml_plugin)

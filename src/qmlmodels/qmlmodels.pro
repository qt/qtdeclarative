TARGET = QtQmlModels
QT = core-private qml-private

QMAKE_DOCS = $$PWD/doc/qtqmlmodels.qdocconf

DEFINES += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES QT_NO_FOREACH

HEADERS += \
    $$PWD/qqmlchangeset_p.h \
    $$PWD/qqmlmodelsmodule_p.h \
    $$PWD/qtqmlmodelsglobal_p.h \
    $$PWD/qtqmlmodelsglobal.h

SOURCES += \
    $$PWD/qqmlchangeset.cpp \
    $$PWD/qqmlmodelsmodule.cpp

qtConfig(qml-object-model) {
    SOURCES += \
        $$PWD/qqmlinstantiator.cpp \
        $$PWD/qqmlobjectmodel.cpp

    HEADERS += \
        $$PWD/qqmlinstantiator_p.h \
        $$PWD/qqmlinstantiator_p_p.h \
        $$PWD/qqmlobjectmodel_p.h
}

qtConfig(qml-table-model) {
    SOURCES += \
        $$PWD/qqmltableinstancemodel.cpp \
        $$PWD/qqmltablemodel.cpp \
        $$PWD/qqmltablemodelcolumn.cpp

    HEADERS += \
        $$PWD/qqmltableinstancemodel_p.h \
        $$PWD/qqmltablemodel_p.h \
        $$PWD/qqmltablemodelcolumn_p.h
}

qtConfig(qml-list-model) {
    SOURCES += \
        $$PWD/qqmllistmodel.cpp \
        $$PWD/qqmllistmodelworkeragent.cpp

    HEADERS += \
        $$PWD/qqmllistmodel_p.h \
        $$PWD/qqmllistmodel_p_p.h \
        $$PWD/qqmllistmodelworkeragent_p.h
}

qtConfig(qml-delegate-model) {
    SOURCES += \
        $$PWD/qqmladaptormodel.cpp \
        $$PWD/qqmldelegatemodel.cpp \
        $$PWD/qqmldelegatecomponent.cpp \
        $$PWD/qqmllistaccessor.cpp \
        $$PWD/qqmllistcompositor.cpp \
        $$PWD/qquickpackage.cpp

    HEADERS += \
        $$PWD/qqmladaptormodel_p.h \
        $$PWD/qqmldelegatemodel_p.h \
        $$PWD/qqmldelegatemodel_p_p.h \
        $$PWD/qqmldelegatecomponent_p.h \
        $$PWD/qqmllistaccessor_p.h \
        $$PWD/qqmllistcompositor_p.h \
        $$PWD/qquickpackage_p.h
}

load(qt_module)

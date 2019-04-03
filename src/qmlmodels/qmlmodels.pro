TARGET = QtQmlModels
QT = core-private qml-private

DEFINES += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES QT_NO_FOREACH

HEADERS += \
    $$PWD/qqmlchangeset_p.h \
    $$PWD/qqmlinstantiator_p.h \
    $$PWD/qqmlinstantiator_p_p.h \
    $$PWD/qqmllistaccessor_p.h \
    $$PWD/qqmllistcompositor_p.h \
    $$PWD/qqmlmodelsmodule_p.h \
    $$PWD/qqmlobjectmodel_p.h \
    $$PWD/qqmltableinstancemodel_p.h \
    $$PWD/qqmltablemodel_p.h \
    $$PWD/qqmltablemodelcolumn_p.h \
    $$PWD/qquickpackage_p.h \
    $$PWD/qtqmlmodelsglobal_p.h \
    $$PWD/qtqmlmodelsglobal.h \

SOURCES += \
    $$PWD/qqmlchangeset.cpp \
    $$PWD/qqmlinstantiator.cpp \
    $$PWD/qqmllistaccessor.cpp \
    $$PWD/qqmllistcompositor.cpp \
    $$PWD/qqmlmodelsmodule.cpp \
    $$PWD/qqmlobjectmodel.cpp \
    $$PWD/qqmltableinstancemodel.cpp \
    $$PWD/qqmltablemodel.cpp \
    $$PWD/qqmltablemodelcolumn.cpp \
    $$PWD/qquickpackage.cpp

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
        $$PWD/qqmldelegatecomponent.cpp

    HEADERS += \
        $$PWD/qqmladaptormodel_p.h \
        $$PWD/qqmldelegatemodel_p.h \
        $$PWD/qqmldelegatemodel_p_p.h \
        $$PWD/qqmldelegatecomponent_p.h
}

load(qt_module)

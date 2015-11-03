contains(QT_CONFIG, no-qml-debug):DEFINES += QT_NO_QML_DEBUGGER

SOURCES += \
    $$PWD/qqmldebug.cpp \
    $$PWD/qqmldebugconnector.cpp \
    $$PWD/qqmldebugservice.cpp \
    $$PWD/qqmldebugserviceinterfaces.cpp \
    $$PWD/qqmlabstractprofileradapter.cpp \
    $$PWD/qqmlprofiler.cpp

HEADERS += \
    $$PWD/qqmldebugconnector_p.h \
    $$PWD/qqmldebugpluginmanager_p.h \
    $$PWD/qqmldebugservice_p.h \
    $$PWD/qqmldebugservicefactory_p.h \
    $$PWD/qqmldebugserviceinterfaces_p.h \
    $$PWD/qqmldebugstatesdelegate_p.h \
    $$PWD/qqmldebug.h \
    $$PWD/qqmlprofilerdefinitions_p.h \
    $$PWD/qqmlabstractprofileradapter_p.h \
    $$PWD/qqmlprofiler_p.h

INCLUDEPATH += $$PWD

option(host_build)

QT       = core-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_TARGET_DESCRIPTION = QML Types Registrar

include(../../tools/shared/shared.pri)

SOURCES += \
    qmltyperegistrar.cpp \
    qmltypesclassdescription.cpp \
    qmltypescreator.cpp

HEADERS += \
    qmltypesclassdescription.h \
    qmltypescreator.h

build_integration.files = qmltypes.prf
build_integration.path = $$[QT_HOST_DATA]/mkspecs/features
prefix_build: INSTALLS += build_integration
else: COPIES += build_integration

load(qt_tool)

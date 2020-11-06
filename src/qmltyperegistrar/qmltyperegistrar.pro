option(host_build)

QT       = core-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_TARGET_DESCRIPTION = QML Types Registrar

# We cannot link against libQmlCompiler as qmltyperegistrar
# has to be built before libQmlCompiler.

INCLUDEPATH += $$PWD/../qmlcompiler

SOURCES += \
    ../qmlcompiler/qqmljsstreamwriter.cpp \
    qmltyperegistrar.cpp \
    qmltypesclassdescription.cpp \
    qmltypescreator.cpp \
    metatypesjsonprocessor.cpp


HEADERS += \
    ../qmlcompiler/qqmljsstreamwriter_p.h \
    qmltypesclassdescription.h \
    qmltypescreator.h \
    metatypesjsonprocessor.h

build_integration.files = qmltypes.prf
build_integration.path = $$[QT_HOST_DATA]/mkspecs/features

prefix_build {
    load(qt_build_paths)
    qmltypes_to_builddir.files = qmltypes.prf
    qmltypes_to_builddir.path = $$MODULE_BASE_OUTDIR/mkspecs/features
    COPIES += qmltypes_to_builddir
    INSTALLS += build_integration
} else {
    COPIES += build_integration
}

load(qt_tool)

TARGET     = QtQuickTest

QMAKE_DOCS = $$PWD/doc/qtqmltest.qdocconf

DEFINES += QT_NO_URL_CAST_FROM_STRING QT_NO_FOREACH
QT = core testlib-private
QT_PRIVATE = quick quick-private qml-private gui core-private

# Testlib is only a private dependency, which results in our users not
# inheriting testlibs's MODULE_CONFIG transitively. Make it explicit.
MODULE_CONFIG += $${QT.testlib.CONFIG}

qtHaveModule(widgets) {
    QT += widgets
    DEFINES += QT_QMLTEST_WITH_WIDGETS
}

SOURCES += \
    $$PWD/quicktest.cpp \
    $$PWD/quicktestresult.cpp

HEADERS += \
    $$PWD/quicktestglobal.h \
    $$PWD/quicktest.h \
    $$PWD/quicktest_p.h \
    $$PWD/quicktestresult_p.h \
    $$PWD/qtestoptions_p.h

qtConfig(qml-debug): DEFINES += QT_QML_DEBUG_NO_WARNING

load(qt_module)

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/Qt/test/qtestroot
QML_IMPORT_NAME = Qt.test.qtestroot
QML_IMPORT_VERSION = 1.0
CONFIG += qmltypes install_qmltypes install_metatypes

# Install qmldir
qmldir.files = $$PWD/qmldir
qmldir.path = $$QMLTYPES_INSTALL_DIR

prefix_build: INSTALLS += qmldir
else: COPIES += qmldir

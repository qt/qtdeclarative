QT = core testlib declarative qml quick qmltest qmldevtools
HEADERSCLEAN_PRI = $${QT.core.sources}/../../tests/auto/other/headersclean/headersclean.pri
isEmpty(QT.core.sources)|!include($$HEADERSCLEAN_PRI) {
    warning("headersclean.pri from QtCore sources not available.  test disabled")
    TEMPLATE=subdirs
}

# shadowing problems in scenegraph, allow it for now
*-g++*: QMAKE_CXXFLAGS -= -Wshadow

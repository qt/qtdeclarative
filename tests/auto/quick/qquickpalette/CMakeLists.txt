qt_add_test(tst_qquickpalette
    SOURCES
        tst_qquickpalette.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

qt_add_test(tst_qquickcolorgroup
    SOURCES
        tst_qquickcolorgroup.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

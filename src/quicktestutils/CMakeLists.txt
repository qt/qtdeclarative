if(TARGET Qt::Quick)
    set (quick_sources
        quick/geometrytestutils.cpp
        quick/geometrytestutils_p.h
        quick/viewtestutils.cpp
        quick/viewtestutils_p.h
        quick/visualtestutils.cpp
        quick/visualtestutils_p.h
    )
endif()

qt_internal_add_module(QuickTestUtilsPrivate
    CONFIG_MODULE_NAME quicktestutilsprivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qml/platforminputcontext_p.h
        qml/platformquirks_p.h
        qml/qmlutils.cpp
        qml/qmlutils_p.h
        qml/testhttpserver.cpp
        qml/testhttpserver_p.h
        ${quick_sources}
    DEFINES
        QT_BUILD_SHARED_QML_TEST_UTILS_LIB
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Test
        Qt::Qml
        Qt::QmlPrivate
)

if(TARGET Qt::Quick)
    qt_internal_extend_target(QuickTestUtilsPrivate
        LIBRARIES
            Qt::Quick
            Qt::QuickPrivate
            Qt::QuickTest
    )
endif()

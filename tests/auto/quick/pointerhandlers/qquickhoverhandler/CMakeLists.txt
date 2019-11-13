# Generated from qquickhoverhandler.pro.

#####################################################################
## tst_qquickhoverhandler Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickhoverhandler
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../../shared/geometrytestutil.cpp ../../shared/geometrytestutil.h
        ../../shared/viewtestutil.cpp ../../shared/viewtestutil.h
        ../../shared/visualtestutil.cpp ../../shared/visualtestutil.h
        tst_qquickhoverhandler.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
    INCLUDE_DIRECTORIES
        ../../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickhoverhandler.pro:<TRUE>:
# OTHER_FILES = "data/lesHoverables.qml"

## Scopes:
#####################################################################

extend_target(tst_qquickhoverhandler CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickhoverhandler CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

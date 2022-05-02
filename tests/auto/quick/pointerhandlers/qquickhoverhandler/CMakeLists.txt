# Generated from qquickhoverhandler.pro.

#####################################################################
## tst_qquickhoverhandler Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickhoverhandler
    SOURCES
        tst_qquickhoverhandler.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickhoverhandler.pro:<TRUE>:
# OTHER_FILES = "data/lesHoverables.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickhoverhandler CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickhoverhandler CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

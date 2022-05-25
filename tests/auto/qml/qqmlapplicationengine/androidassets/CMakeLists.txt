qt_internal_add_test(tst_androidassets
    SOURCES
        tst_androidassets.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# add qml/*.qml files as assets instead of resources

file(
    COPY qml/main.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/android-build/assets/qml/")

file(
    COPY qml/pages/MainPage.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/android-build/assets/qml/pages/")

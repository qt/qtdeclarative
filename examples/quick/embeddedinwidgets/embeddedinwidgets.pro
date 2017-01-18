TEMPLATE = app
QT += widgets quick

INCLUDEPATH += /home/user/qtwebdriver/out/dist/desktop/release/Test
INCLUDEPATH += /home/user/qtwebdriver/out/dist/desktop/release/h
LIBS += -L/home/user/qtwebdriver/out/dist/desktop/release/libs
LIBS += -lchromium_base -lWebDriver_core -lWebDriver_extension_qt_base -lWebDriver_extension_qt_quick -lWebDriver_extension_qt_quick_web -lWebDriver_extension_qt_web
DEFINES += QT_NO_SAMPLES="1"

SOURCES += main.cpp

OTHER_FILES += main.qml TextBox.qml

RESOURCES += \
    embeddedinwidgets.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/embeddedinwidgets
INSTALLS += target

TEMPLATE = app
TARGET = texteditor
QT += quick quickcontrols2
qtHaveModule(widgets): QT += widgets

cross_compile: DEFINES += QT_EXTRA_FILE_SELECTOR=\\\"touch\\\"

SOURCES += \
    texteditor.cpp

OTHER_FILES += \
    qml/*.qml

RESOURCES += \
    texteditor.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/texteditor
INSTALLS += target

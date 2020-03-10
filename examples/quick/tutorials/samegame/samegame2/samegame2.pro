TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

RESOURCES += \
    samegame2.qrc \
    ../shared/pics/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/tutorials/samegame/samegame2
INSTALLS += target

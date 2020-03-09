TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

RESOURCES += \
    samegame1.qrc \
    ../shared/pics/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/tutorials/samegame/samegame1
INSTALLS += target

TEMPLATE = app

QT += qml quick
CONFIG += c++11

SOURCES += main.cpp

RESOURCES += qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtlabscontrols/chattutorial/chapter1-settingup
INSTALLS += target

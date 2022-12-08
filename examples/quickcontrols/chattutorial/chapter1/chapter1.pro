TEMPLATE = app

QT += qml quick
CONFIG += c++11

SOURCES += main.cpp

resources.files = main.qml
resources.prefix = qt/qml/chapter1/
RESOURCES += resources \
    qtquickcontrols2.conf

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/chattutorial/chapter1
INSTALLS += target

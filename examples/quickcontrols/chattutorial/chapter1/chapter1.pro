TEMPLATE = app

QT += qml quick

SOURCES += main.cpp

resources.files = \
    Main.qml \
    qmldir
resources.prefix = qt/qml/chattutorial/
RESOURCES += resources \
    qtquickcontrols2.conf

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/chattutorial/chapter1
INSTALLS += target

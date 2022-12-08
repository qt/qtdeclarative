TEMPLATE = app

QT += qml quick
CONFIG += c++11

SOURCES += main.cpp

resources.files = \
    images/Albert_Einstein.png \
    images/Albert_Einstein@2x.png \
    images/Albert_Einstein@3x.png \
    images/Albert_Einstein@4x.png \
    images/Ernest_Hemingway.png \
    images/Ernest_Hemingway@2x.png \
    images/Ernest_Hemingway@3x.png \
    images/Ernest_Hemingway@4x.png \
    images/Hans_Gude.png \
    images/Hans_Gude@2x.png \
    images/Hans_Gude@3x.png \
    images/Hans_Gude@4x.png \
    main.qml
resources.prefix = qt/qml/chapter2/
RESOURCES += resources \
    qtquickcontrols2.conf

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/chattutorial/chapter2
INSTALLS += target

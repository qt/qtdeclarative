TEMPLATE = app

QT += qml quick sql
CONFIG += c++11

HEADERS += sqlcontactmodel.h \
    sqlconversationmodel.h

SOURCES += main.cpp \
    sqlcontactmodel.cpp \
    sqlconversationmodel.cpp

resources.files = \
    ContactPage.qml \
    ConversationPage.qml \
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
resources.prefix = qt/qml/chapter4/
RESOURCES += resources \
    qtquickcontrols2.conf

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/chattutorial/chapter4
INSTALLS += target

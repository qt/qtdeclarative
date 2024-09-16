TEMPLATE = app
TARGET = contactlist
QT += quick

CONFIG += qmltypes

QML_IMPORT_PATH = $$pwd/.
QML_IMPORT_NAME = contactlist
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += \
    contactmodel.h

SOURCES += \
    main.cpp \
    contactmodel.cpp

qml_resources.files = \
    qmldir \
    ContactDelegate.ui.qml \
    ContactDialog.qml \
    ContactForm.ui.qml \
    ContactList.qml \
    ContactView.ui.qml \
    designer/Backend/ContactModel.qml \
    SectionDelegate.ui.qml

qml_resources.prefix = /qt/qml/contactlist

RESOURCES += qml_resources

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD/designer

OTHER_FILES += \
    designer/Backend/*.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/contactlist
INSTALLS += target

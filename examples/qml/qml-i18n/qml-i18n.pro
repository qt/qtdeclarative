TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

RESOURCES += qml-i18n.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qml/qml-i18n
INSTALLS += target

QM_FILES_RESOURCE_PREFIX = /qt/qml/translated/i18n/
CONFIG += lrelease embed_translations

TRANSLATIONS += \
    i18n/base.ts \
    i18n/qml_en.ts \
    i18n/qml_en_AU.ts \
    i18n/qml_fr.ts

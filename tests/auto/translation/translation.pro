CONFIG += testcase
TARGET = tst_translation
SOURCES += tst_translation.cpp

macos:CONFIG -= app_bundle

QT += testlib gui-private quicktemplates2-private

include (../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/*.qml

# We only want to run lrelease, which is why we use EXTRA_TRANSLATIONS.
EXTRA_TRANSLATIONS = qtbase_fr.ts
# Embed the translations in a qrc file.
CONFIG += lrelease embed_translations

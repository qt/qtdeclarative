load(qttest_p4)
QT += declarative
macx:CONFIG -= app_bundle
SOURCES += tst_qjsengine.cpp

wince* {
    DEFINES += SRCDIR=\\\"./\\\"
} else:!symbian {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

wince*|symbian: {
   addFiles.files = script
   addFiles.path = .
   DEPLOYMENT += addFiles
}

symbian: {
   TARGET.UID3 = 0xE0340006
   DEFINES += SYMBIAN_SRCDIR_UID=$$lower($$replace(TARGET.UID3,"0x",""))
}

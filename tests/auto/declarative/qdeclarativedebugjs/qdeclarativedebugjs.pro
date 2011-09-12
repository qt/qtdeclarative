load(qttest_p4)
QT += declarative network script declarative-private
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h

SOURCES +=     tst_qdeclarativedebugjs.cpp \
            ../shared/debugutil.cpp

INCLUDEPATH += ../shared

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

symbian {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
}

OTHER_FILES =   data/test.qml \
                data/test.js


CONFIG += parallel_test

load(qttest_p4)
QT += declarative network script declarative-private
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h

SOURCES +=     tst_qdeclarativedebugjs.cpp \
            ../shared/debugutil.cpp

INCLUDEPATH += ../shared

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

OTHER_FILES =   data/test.qml \
                data/test.js


CONFIG += parallel_test
#temporary
CONFIG += insignificant_test

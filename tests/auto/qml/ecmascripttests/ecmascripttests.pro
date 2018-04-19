CONFIG += testcase
TARGET = tst_ecmascripttests
QT += testlib
macos:CONFIG -= app_bundle
SOURCES += tst_ecmascripttests.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

TESTSCRIPT=$$PWD/test262.py
isEmpty(V4CMD): V4CMD = qmljs

# The ES test suite takes approximately 5 mins to run, on a fairly
# vanilla developer machine, so the default watchdog timer kills the
# test some of the time.  Fix by raising time-out to 400s when
# invoking tst_ecmascripttests:
checkenv.name = QTEST_FUNCTION_TIMEOUT
checkenv.value = 400000
QT_TOOL_ENV += checkenv

checkjittarget.target = check-jit
checkjittarget.commands = python $$TESTSCRIPT --command=$$V4CMD --parallel --with-test-expectations --update-expectations
checkjittarget.depends = all
QMAKE_EXTRA_TARGETS += checkjittarget

checkmothtarget.target = check-interpreter
checkmothtarget.commands = python $$TESTSCRIPT --command=\"$$V4CMD --interpret\" --parallel --with-test-expectations
checkmothtarget.depends = all
QMAKE_EXTRA_TARGETS += checkmothtarget


load(qttest_p4)
QT = core qmldevtools-private
macx:CONFIG -= app_bundle

SOURCES += tst_compile.cpp

CONFIG += parallel_test

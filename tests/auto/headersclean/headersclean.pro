load(qttest_p4)
SOURCES  += tst_headersclean.cpp
QT = core

contains(QT_CONFIG,declarative): QT += declarative

load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui
macx:CONFIG -= app_bundle

SOURCES += tst_qquickmultipointtoucharea.cpp

importFiles.files = data
importFiles.path = .
DEPLOYMENT += importFiles

QT += core-private gui-private declarative-private

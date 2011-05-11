load(qttest_p4)
macx:CONFIG -= app_bundle

SOURCES += tst_v8.cpp 

CONFIG += parallel_test

LIBS += -L../../../../src/v8/
macx:CONFIG(debug, debug|release) {
    LIBS += -lv8_debug 
} else {
    LIBS += -lv8 
}


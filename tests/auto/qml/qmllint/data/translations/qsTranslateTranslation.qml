import QtQuick

Item {
    property string qsTranslateTest: qsTranslate("context", "Hello")
    property string qsTranslateNoopTest: QT_TRANSLATE_NOOP("context", "Hello")
}

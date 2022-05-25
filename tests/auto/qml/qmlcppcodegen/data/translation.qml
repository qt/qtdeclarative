pragma Strict
import QtQml

QtObject {
    // Force the engine to generate script bindings by appending an empty string to each one.

    property string translate2: qsTranslate("c", "s") + ""
    property string translate3: qsTranslate("c", "s", "d")  + ""
    property string translate4: qsTranslate("c", "s", "d", 4) + ""

    property string translateNoop2: QT_TRANSLATE_NOOP("c", "s") + ""
    property string translateNoop3: QT_TRANSLATE_NOOP("c", "s", "d") + ""

    property string tr1: qsTr("s") + ""
    property string tr2: qsTr("s", "d") + ""
    property string tr3: qsTr("s", "d", 4) + ""

    property string trNoop1: QT_TR_NOOP("s") + ""
    property string trNoop2: QT_TR_NOOP("s", "d") + ""

    property string trId1: qsTrId("s") + ""
    property string trId2: qsTrId("s", 4) + ""

    property string trIdNoop1: QT_TRID_NOOP("s") + ""
}

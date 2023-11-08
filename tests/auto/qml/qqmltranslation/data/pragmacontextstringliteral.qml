import QtQml 2.12

pragma Translator: "contextSetWithPragmaStringLiteral"

QtObject {
    property string german1: qsTr("English in translation")
    property string german2: qsTranslate("setContext","English in translation")
}

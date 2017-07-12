import QtQml 2.0

TranslationChangeBase {
    baseProperty: "do not translate"
    property string text1: qsTr("translate me")
    function weDoTranslations() {
        return qsTr("translate me")
    }
    property string text2: weDoTranslations()
}

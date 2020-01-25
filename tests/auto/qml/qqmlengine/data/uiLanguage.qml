import QtQml 2.15
QtObject {
    property string chosenLanguage: Qt.uiLanguage
    property string textToTranslate: {
        numberOfTranslationBindingEvaluations++;
        return qsTr("Translate me maybe");
    }
    property int numberOfTranslationBindingEvaluations: 0
}

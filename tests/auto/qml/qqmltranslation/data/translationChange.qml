import QtQuick 2.0

TranslationChangeBase {
    id: root

    baseProperty: "do not translate"
    property string text1: qsTr("translate me")
    function weDoTranslations() {
        return qsTr("translate me")
    }
    property string text2: weDoTranslations()
    property string text3

    states: [
        State {
            name: "default"
            when: 1 == 1
            PropertyChanges {
                target: root
                text3: qsTr("translate me")
            }
        }
    ]
}

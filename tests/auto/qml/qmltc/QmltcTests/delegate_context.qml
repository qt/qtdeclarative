import QtQuick

Text {
    id: root
    text: "hello"

    ListView {
        id: listView
        model: 1
        anchors.fill: parent // QTBUG-104780

        delegate: Text { // QV4::CompiledData::Object::IsComponent
            id: listViewDelegate
            text: root.text + " delegate"
            // TODO: listView.text + " delegate" doesn't work - why?

            Text {
                id: textInsideDelegate
                text: listViewDelegate.text + " text"
            }
        }
    }
}

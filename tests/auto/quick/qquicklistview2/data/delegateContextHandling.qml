pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window

Item {
    id: win
    height: 640
    width: 480

    property string currentModel: 'foo'

    function toggle() : Item {
        var ret = listView.itemAtIndex(0);
        win.currentModel = win.currentModel === 'foo' ? 'bar' : 'foo'

        switch (win.currentModel) {
        case 'foo':
            if (listView.model) {
                listView.model.destroy()
            }
            listView.model = fooModelComponent.createObject(win)
            break

        case 'bar':
            if (listView.model) {
                listView.model.destroy()
            }
            listView.model = barModelComponent.createObject(win)
            break
        }

        return ret;
    }

    Component {
        id: fooModelComponent
        ListModel {
            ListElement { textValue: "foo1" }
        }
    }

    Component {
        id: barModelComponent
        ListModel {
            ListElement { textValue: "bar1" }
        }
    }

    ListView {
        states: [
            State {
                when: win.currentModel === 'bar'
                PropertyChanges {
                    listView.section.property: 'sectionProp'
                }
            }
        ]

        id: listView
        model: fooModelComponent.createObject(win)
        anchors.fill: parent

        section.delegate: Text {
            required property string section
            text: section
        }

        delegate: Text {
            id: delg
            text: delg.textValue
            required property string textValue
        }
    }
}

import QtQuick
import QtQml.Models

Item {
    ListModel {
        id: myModel
        ListElement { role_display: "One"; role_value: 0; }
        ListElement { role_display: "One"; role_value: 2; }
        ListElement { role_display: "One"; role_value: 3; }
        ListElement { role_display: "One"; role_value: 4; }
        ListElement { role_details: "Two"; role_value: 5; }
        ListElement { role_details: "Three"; role_value: 6; }
        ListElement { role_details: "Four"; role_value: 7; }
        ListElement { role_details: "Five"; role_value: 8; }
        ListElement { role_details: "Six"; role_value: 9; }
        ListElement { role_keyID: "Seven"; role_value: 10; }
        ListElement { role_keyID: "Eight"; role_value: 11; }
        ListElement { role_keyID: "hello"; role_value: 12; }
    }

    DelegateModel {
        id: displayDelegateModel
        delegate: Text { text: role_display }
        model: myModel
        groups: [
            DelegateModelGroup {
                includeByDefault: false
                name: "displayField"
            }
        ]
        filterOnGroup: "displayField"
        Component.onCompleted: {
            var rowCount = myModel.count;
            items.remove(0, rowCount);
            for (var i = 0; i < rowCount; i++) {
                var entry = myModel.get(i);
                if (entry.role_display) {
                    items.insert(entry, "displayField");
                }
            }
        }
    }

    ListView {
        model: displayDelegateModel
    }
}


import QtQuick
import QtQuick.Controls.Basic

Item {
    width: 640
    height: 480
    ListView {
        id: lst_view
        objectName: "list"
        anchors.fill: parent
        clip: true
        smooth: true
        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.DragAndOvershootBounds
        cacheBuffer: 50
        section.property: "sectionId"
        section.criteria: ViewSection.FullString
        section.delegate: Item {
            required property string section
            width: 640
            height: 32
            Text {
                text: section
                font.pixelSize: 30
                color: "black"
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
            }
        }
        Rectangle {
            id: top_over
            gradient: Gradient {
                GradientStop { position: 1.0; color: "#0000ff00" }
                GradientStop { position: 0.0; color: "#ff00ff00" }
            }
            x : 0
            y : 0
            width: 640
            height: 30
            opacity: (lst_view.verticalOvershoot < 0.0) ? Math.max(0.0, (Math.abs(lst_view.verticalOvershoot) / (lst_view.height / 2))) : 0.0;
        }
        Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0000ff00" }
                GradientStop { position: 1.0; color: "#ff00ff00" }
            }
            x : 0
            y : 480 - 30
            width: 640
            height: 30
            opacity: (0.0 < lst_view.verticalOvershoot) ? Math.max(0.0, (Math.abs(lst_view.verticalOvershoot) / (lst_view.height / 2))) : 0.0;
        }
        delegate: Text {
            font.pixelSize: 36
            color: "black"
            text: title + " y " + y + " height " + height
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            width: 640
            height: 100
        }
        model: ListModel {
            id: list_info
        }
        Component.onCompleted: {
            for(var i = 1; i <= 40; i++)
            {
                list_info.append({ title: "Item " + i, sectionId: "Section " + ((i <= 20)? "A": (i <= 30)? "B": "C") });
            }
            lst_view.model = list_info;
        }
    }
}

import QtQuick
import QtQuick.Layouts

Rectangle {
    width: 640
    height: 480
    visible: true

    ListView {
        id: idObjects
        objectName: "objects"
        anchors.fill: parent

        spacing: 1
        model : 50

        clip : true

        contentWidth: contentItem && contentItem.childrenRect ? contentItem.childrenRect.width : -1
        contentHeight: contentItem && contentItem.childrenRect ? contentItem.childrenRect.height : -1

        delegate: Item {
            id: idSelf

            property ListView ownerListView :  ListView.view
            property int delegateIndex: index

            width: ownerListView.width
            height: idHtmlRenderer.height + 4

            Rectangle {
                anchors.fill : parent
                anchors.topMargin: 1
                anchors.bottomMargin: 1
                color: "#FFB0B0B0"
            }

            Text{
                id: idHtmlRenderer
                anchors.top : parent.top
                anchors.left : parent.left
                anchors.right : parent.right

                anchors.leftMargin: 4
                anchors.rightMargin: 4
                anchors.topMargin: 4
                anchors.bottomMargin: 4

                textFormat: Text.RichText
                wrapMode:Text.Wrap

                color: "#000000"
                font.family: "Consolas"
                font.pointSize:  12

                text: "<table border='0' cellspacing='0' cellpadding='0'  width='*' style='table-layout:fixed; border-spacing: 0;'><tr><td style='text-align: center; collapse;vertical-align: middle; padding: 2px; white-space:normal; word-wrap:break-word;'  width='50'><span style='display:inline-block;height:20px;width:20px;background-color:#00ffff;color:#00ffff;'>[]</span> </td><td style='text-align: left; collapse;vertical-align: middle; padding: 2px; white-space:normal; word-wrap:break-word;'  width='350'>Cyan<br>RGB: r=0, g=255, b=255</td></tr></table>"

                onWidthChanged:{
                    forceLayout()
                }
            }
        }
    }
}

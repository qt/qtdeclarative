import QtQuick
import Test

ListView {
    id: gadgetListView

    width: 300
    height: 300

    model: GadgetModel {}

    Component {
        id: sectionHeading
        Rectangle {
            width: ListView.view.width
            height: childrenRect.height
            color: "lightsteelblue"

            required property string section

            Text {
                text: parent.section
                font.bold: true
                font.pixelSize: 15
            }
        }
    }

    header: Text {
        text: "MyGadgetList"
        color: "green"
        font.pixelSize: 18
    }

    delegate: Text {
        required property var gadget
        required property int index
        text: gadget? (gadget.name + "/" + gadget.size) : ("gadget " + index + " unavailable")
    }

    section {
        property: "gadget.size"
        criteria: ViewSection.FullString
        delegate: sectionHeading
    }
}

// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

//! [parent begin]
Rectangle {
//! [parent begin]

    //! [inherited properties]
    width: 320; height: 240
    color: "lightblue"
    focus: true
    //! [inherited properties]

    //! [custom properties]
    property int counter
    property real area: 100.45
    //! [custom properties]

    //! [property types]
    property int number
    property real volume: 100.45
    property date today: "2011-01-01"
    property color background: "yellow"
    //! [property types]


//! [grouped properties]
Text {
    //dot notation
    font.pixelSize: 12
    font.bold: true
}

Text {
    //group notation
    font {pixelSize: 12; bold: true}
}
//! [grouped properties]


//! [property binding]
Rectangle {
    width: parent.width
}
//! [property binding]

//! [property assignment]
Rectangle {
    Component.onCompleted: {
        width = 150
    }
}
//! [property assignment]

Rectangle {
    //placeholder slider
    id: slider
    property real value
}
Rectangle {
    //placeholder system
    id: system
    property real brightness
}
//! [binding element]
Binding {
    target: system
    property: "brightness"
    value: slider.value
}
//! [binding element]

Rectangle {
    //placeholder warning
    id: warning
    color: "red"
}
//! [PropertyChanges element]
Rectangle {
    id: rectangle

    states: State {
        name: "WARNING"
        PropertyChanges {
            rectangle.color: warning.color
        }
    }
}
//! [PropertyChanges element]

//! [list property]
Item {
    id: multistate
    states: [
        State {name: "FETCH"},
        State {name: "DECODE"},
        State {name: "EXECUTE"}
    ]
}
//! [list property]
//! [single property]
Item {
    id: monostate
    states: State {name: "RUNNING"}
}
//! [single property]

Item {
    id: printstate
//! [print list property]
    Component.onCompleted: console.log (multistate.states[0].name)
//! [print list property]
}

//! [JavaScript sample]
function calculateArea(width, height) {
    return (width * height) * 0.5
}

Rectangle {
    width: 150; height: 75
    property real area: calculateArea(width, height)
    property real parentArea: calculateArea(parent.width,parent.height)
    color: { if (area > parentArea) "blue"; else "red" }
}
//! [JavaScript sample]

//! [id property]
Rectangle {
    id: container
    width: 100; height: 100
    Rectangle {
        width: parent.width; height: parent.height
    }
}
Rectangle {
    width: container.width; height: container.height
}
//! [id property]

//! [default property]
Item {
    Text {}
    Rectangle {}
    Timer {}
}

Item {
    //without default property
    children: [
        Text {},
        Rectangle {}
    ]
    resources: [
        Timer {}
    ]
}
//! [default property]

//! [state default]
State {
    changes: [
        PropertyChanges {},
        PropertyChanges {}
    ]
}

State {
    PropertyChanges {}
    PropertyChanges {}
}
//! [state default]

//! [object binding]
Rectangle {

    id: parentrectangle
    gradient:
        Gradient { //not a child of parentrectangle

            //generates a TypeError
            //Component.onCompleted: console.log(parent.width)
        }

    //child of parentrectangle
    Rectangle {property string name: "childrectangle"}

    //prints "childrectangle"
    Component.onCompleted: console.log(children[0].name)
}
//! [object binding]

//! [list attached property]
Component {
    id: listdelegate
    Text {
        text: "Hello"
        color: ListView.isCurrentItem ? "red" : "blue"
    }
}
ListView {
    delegate: listdelegate
}
//! [list attached property]

//! [attached signal handler]
Item {
    Keys.onPressed: console.log("Key Press Detected")
    Component.onCompleted: console.log("Completed initialization")
}
//! [attached signal handler]

//! [alias usage]
Button {
    id: textbutton
    buttonLabel: "Click Me!"
}
//! [alias usage]

//! [image alias]
Button {
    id: imagebutton
    buttonImage.source: "http://qt.nokia.com/logo.png"
    buttonLabel: buttonImage.source
}
//! [image alias]

Item {
id: widget

//! [alias complete]
property alias widgetLabel: label

//will generate an error
//widgetLabel.text: "Initial text"

//will generate an error
//property alias widgetLabelText: widgetLabel.text

Component.onCompleted: widgetLabel.text = "Alias completed Initialization"
//! [alias complete]

    Text {id: label}
}

//![alias overwrite]
Rectangle {
    id: coloredrectangle
    property alias color: bluerectangle.color
    color: "red"

    Rectangle {
        id: bluerectangle
        color: "#1234ff"
    }

    Component.onCompleted: {
        console.log (coloredrectangle.color)    //prints "#1234ff"
        setInternalColor()
        console.log (coloredrectangle.color)    //prints "#111111"
        coloredrectangle.color = "#884646"
        console.log (coloredrectangle.color)    //prints #884646
    }

    //internal function that has access to internal properties
    function setInternalColor() {
        color = "#111111"
    }
}
//![alias overwrite]
//! [parent end]
}
//! [parent end]
//! [document]

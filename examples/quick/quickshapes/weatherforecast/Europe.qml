// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
Item {
    id: map

    implicitWidth: mapShape.implicitWidth
    implicitHeight: mapShape.implicitHeight

    Europe_generated {
        id: mapShape
    }

    property int containsMode: Shape.FillContains

    property bool zoomedIn: false
    signal zoomTo(Item child, string name, var childRect, var textRect)

    property var lookupTable: {
        "fi"         : "Finland"
        ,"se"         : "Sweden"
        ,"dk"         : "Denmark"
        ,"gb-nir"     : "Northern Ireland"
        ,"gb-main"    : "Great Britain"
        ,"ie"         : "Ireland"
        ,"nl"         : "The Netherlands"
        ,"be"         : "Belgium"
        ,"lu"         : "Luxembourg"
        ,"de"         : "Germany"
        ,"fr"         : "France"
        ,"es"         : "Spain"
        ,"pt"         : "Portugal"
        ,"at"         : "Austria"
        ,"it"         : "Italy"
        ,"gr"         : "Greece"
        ,"ee"         : "Estonia"
        ,"lv"         : "Latvia"
        ,"lt"         : "Lithuania"
        ,"pl"         : "Poland"
        ,"cz"         : "Czechia"
        ,"sk"         : "Slovakia"
        ,"si"         : "Slovenia"
        ,"hu"         : "Hungary"
        ,"ro"         : "Romania"
        ,"bg"         : "Bulgaria"
        ,"cy"         : "Cyprus"
        ,"mt"         : "Malta"
        ,"hr"         : "Croatia"
        ,"ba"         : "Bosnia and Herzegovina"
        ,"me"         : "Montenegro"
        ,"rs"         : "Serbia"
        ,"rs-km"      : "Kosovo"
        ,"mk"         : "North Makedonia"
        ,"al"         : "Albania"
        ,"is"         : "Iceland"
        ,"by"         : "Belarus"
        ,"no"         : "Norway"
        ,"ua"         : "Ukraine"
        ,"ch"         : "Switzerland"
        ,"md"         : "Moldova"
        ,"ad"         : "Andorra"
        ,"mc"         : "Monaco"
        ,"li"         : "Liechtenstein"
        ,"ru-kgd"     : "Kaliningrad"
        ,"im"         : "Isle of Man"
        ,"fo"         : "Faroe Islands"
    }

    property var overrideRects: {
        "fi": {x: 6616, y: 1996, width: 1920, height: 800, rotation: 65},
        "no": {x: 4794, y: 1967, width: 3360, height: 680, rotation: -65},
        "se": {x: 5739, y: 2560, width: 1920, height: 800, rotation: -80},
        "it": {x: 5734, y: 6483, width: 1680, height: 440, rotation: 45},
        "pt": {x: 3105, y: 6783, width: 960, height: 320, rotation: -70},
        "md": {x: 8294, y: 5472, width: 480, height: 200, rotation: 45},
        "rs": {x: 7233, y: 6168, width: 840, height: 320, rotation: 45},
        "gb-main": {x: 3943, y: 3998, width: 1800, height: 800, rotation: 75},
        "ba": {x: 6570, y: 6289, width: 1440, height: 200, rotation: 0},
        "al": {x: 7312, y: 6830, width: 600, height: 200, rotation: 75},
        "be": {x: 5237, y: 5126, width: 600, height: 200, rotation: 40},
        "hr": {x: 6705, y: 6053, width: 840, height: 200, rotation: 0},
        "at": {x: 6191, y: 5556, width: 1080, height: 440, rotation: -25},
        "cy": {x: 9509, y: 7497, width: 480, height: 200, rotation: -35},
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        property Item currentChild: null
        property color prevColor
        property color selectedColor: "#dbd6c8"

        acceptedButtons: Qt.LeftButton

        function traverseChildren(item, x, y) {
            let p = item.mapFromItem(mouseArea, x, y)
            if (item.objectName) {
                if (item.contains(p))
                    return item;
            }
            for (var i = 0; i < item.children.length; i++) {
                let r = traverseChildren(item.children[i], x, y)
                if (r)
                    return r;
            }
            return null;
        }

        function findChild(x, y) {
            return traverseChildren(map, x, y)
        }

        function findMatchingPath(node) {
            if (!node)
                return null
            let pathName = "svg_path:" + node.objectName
            for (var i = 0; i < node.data.length; i++) {
                let child = node.data[i]
                if (child.objectName === pathName)
                    return child
            }
            return null
        }

        onPressed: (mouse)=> {
                       if (!map.zoomedIn) {
                           currentChild = findChild(mouse.x, mouse.y)
                           let path = findMatchingPath(currentChild)
                           if (path) {
                               prevColor = path.fillColor
                               path.fillColor = selectedColor
                               currentChild.z = 1
                           }
                       }
                   }

        onPositionChanged: (mouse)=> {
                               if (currentChild) {
                                   let localPos = currentChild.mapFromItem(mouseArea, mouse.x, mouse.y)
                                   let path = findMatchingPath(currentChild)
                                   if (path) {
                                       if (currentChild.contains(localPos)) {
                                           path.fillColor = selectedColor
                                       } else {
                                           path.fillColor = prevColor
                                       }
                                   }
                               }
                           }

        onReleased: (mouse) => {
                        if (map.zoomedIn) {
                            zoomTo(null, null, null, null)
                            zoomedIn = false
                            if (currentChild) {
                                let path = findMatchingPath(currentChild)
                                if (path)
                                    path.fillColor = prevColor
                                currentChild.z = 0
                                currentChild = null
                            }
                        } else if (currentChild) {
                            let localPos = currentChild.mapFromItem(mouseArea, mouse.x, mouse.y)
                            if (currentChild.contains(localPos)) {
                                let br = currentChild.boundingRect
                                let rot = 0
                                let or = overrideRects[currentChild.objectName]
                                if (or) {
                                    rot = or.rotation
                                    let r = currentChild.mapToItem(map, or.x, or.y, or.width, or.height)
                                    or = {x: r.x, y: r.y, width: r.width, height: r.height, rotation: rot}
                                }
                                let rect = currentChild.mapToItem(map, br)
                                zoomTo(currentChild, lookupTable[currentChild.objectName], rect, or )
                                zoomedIn = true
                            }
                        }
                    }
    }
}

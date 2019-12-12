/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.1

Rectangle{
    id: root
    width:200
    height:200

    TestUtil {
        id: util
    }

    SignalSpy {
        id: spyX
        target: container2
        signalName: "posXChanged"
    }
    SignalSpy {
        id: spyY
        target: container2
        signalName: "posYChanged"
    }

    Rectangle {
        id:container
        width:20
        height:20
        color: "blue"
        MouseArea {
            id:mouseArea; anchors.fill : parent
            drag.maximumX: 180
            drag.maximumY: 180
            drag.minimumX: 0
            drag.minimumY: 0
            drag.target: parent
        }
    }

    Rectangle {
        id: container2
        x: 25
        y: 25
        width:100
        height:100
        color: "red"
        property bool updatePositionWhileDragging: false
        property var posX: 0
        property var posY: 0

        function reset() {
            fakeHandle.x = 0
            fakeHandle.y = 0
            spyX.clear()
            spyY.clear()
        }

        Binding {
            when: container2.updatePositionWhileDragging
            target: container2
            property: "posX"
            value: fakeHandle.x
        }

        Binding {
            when: container2.updatePositionWhileDragging
            target: container2
            property: "posY"
            value: fakeHandle.y
        }

        Item { id: fakeHandle }

        MouseArea {
            anchors.fill : container2
            drag.maximumX: 180
            drag.maximumY: 180
            drag.minimumX: 0
            drag.minimumY: 0
            drag.target: fakeHandle

            onReleased: if (!container2.updatePositionWhileDragging) {
                            container2.posX = mouse.x;
                            container2.posY = mouse.y
                        }
        }
    }

    Component {
        id: nestedFlickableComponent

        Flickable {
            objectName: "outerFlickable"
            width: parent.width
            height: parent.height
            contentWidth: 400
            contentHeight: 400

            property alias innerFlickable: innerFlickable

            Flickable {
                id: innerFlickable
                objectName: "innerFlickable"
                width: parent.width
                height: parent.height
                contentWidth: 400
                contentHeight: 400

                Rectangle {
                    width: 400
                    height: 400
                    gradient: Gradient {
                        GradientStop { position: 0; color: "salmon" }
                        GradientStop { position: 0; color: "navajowhite" }
                    }
                }
            }
        }
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: mouseAreaComponent

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
        }
    }

    TestCase {
        name:"mouserelease"
        when:windowShown
        function test_mouseDrag() {
            mouseDrag(container, 10, 10, util.dragThreshold * 2, util.dragThreshold * 3);
            compare(container.x, util.dragThreshold - 1);
            compare(container.y, util.dragThreshold * 2 - 1);
        }

        function test_doSomethingWhileDragging() {
            container2.updatePositionWhileDragging = false
            // dx and dy are superior to 3 times util.dragThreshold.
            // but here the dragging does not update posX and posY
            // posX and posY are only updated on mouseRelease
            container2.reset()
            mouseDrag(container2, container2.x + 10, container2.y + 10, 10*util.dragThreshold, 10*util.dragThreshold);
            compare(spyX.count, 1)
            compare(spyY.count, 1)

            container2.updatePositionWhileDragging = true
            // dx and dy are superior to 3 times util.dragThreshold.
            // 3 intermediate mouseMove when dragging
            container2.reset()
            mouseDrag(container2, container2.x + 10, container2.y + 10, 10*util.dragThreshold, 10*util.dragThreshold);
            compare(spyX.count, 3)
            compare(spyY.count, 3)

            // dx and dy are inferior to 3 times util.dragThreshold.
            // No intermediate mouseMove when dragging, only one mouseMove
            container2.reset()
            mouseDrag(container2, container2.x + 10, container2.y + 10, 2*util.dragThreshold, 2*util.dragThreshold);
            compare(spyX.count, 1)
            compare(spyY.count, 1)

            // dx is superior to 3 times util.dragThreshold.
            // 3 intermediate mouseMove when dragging on x axis
            // no move on the y axis
            container2.reset()
            mouseDrag(container2, container2.x + 10, container2.y + 10, 10*util.dragThreshold, 0);
            compare(spyX.count, 3)
            compare(spyY.count, 0)

            // dy is inferior to 3 times util.dragThreshold.
            // No intermediate mouseMove when dragging, only one mouseMove on y axis
            // no move on the x axis
            container2.reset()
            mouseDrag(container2, container2.x + 10, container2.y + 10, 0, 2*util.dragThreshold);
            compare(spyX.count, 0)
            compare(spyY.count, 1)
        }

        function test_dragAxis_data() {
            return [
                { tag: "horizontal" },
                { tag: "vertical" }
            ]
        }

        // mouseDrag() should not drag along an axis if the distance passed in for
        // that axis was 0. Doing so can interfere with tests for an item that e.g.
        // handles horizontal flicks which is within e.g. a Flickable that handles
        // vertical flicks.
        function test_dragAxis(data) {
            let horizontal = data.tag === "horizontal"

            let outerFlickable = createTemporaryObject(nestedFlickableComponent, root)
            verify(outerFlickable)
            // We want the outer flickable to use the opposite flick direction of the inner one,
            // as the inner one has the direction that we're interested in testing.
            outerFlickable.flickableDirection = horizontal ? Flickable.VerticalFlick : Flickable.HorizontalFlick

            let innerFlickable = outerFlickable.innerFlickable
            verify(innerFlickable)
            let horizontalFlickable = null
            let verticalFlickable = null
            if (horizontal) {
                innerFlickable.flickableDirection = Flickable.HorizontalFlick
                horizontalFlickable = innerFlickable
                verticalFlickable = outerFlickable
            } else {
                innerFlickable.flickableDirection = Flickable.VerticalFlick
                horizontalFlickable = outerFlickable
                verticalFlickable = innerFlickable
            }

            let movingHorizontallySpy = createTemporaryObject(signalSpyComponent, root,
                { target: horizontalFlickable, signalName: "movingHorizontallyChanged" })
            verify(movingHorizontallySpy)
            verify(movingHorizontallySpy.valid)

            let movingVerticallySpy = createTemporaryObject(signalSpyComponent, root,
                { target: verticalFlickable, signalName: "movingVerticallyChanged" })
            verify(movingVerticallySpy)
            verify(movingVerticallySpy.valid)

            let flickingHorizontallySpy = createTemporaryObject(signalSpyComponent, root,
                { target: horizontalFlickable, signalName: "flickingHorizontallyChanged" })
            verify(flickingHorizontallySpy)
            verify(flickingHorizontallySpy.valid)

            let flickingVerticallySpy = createTemporaryObject(signalSpyComponent, root,
                { target: verticalFlickable, signalName: "flickingVerticallyChanged" })
            verify(flickingVerticallySpy)
            verify(flickingVerticallySpy.valid)

            let contentXSpy = createTemporaryObject(signalSpyComponent, root,
                { target: horizontalFlickable, signalName: "contentXChanged" })
            verify(contentXSpy)
            verify(contentXSpy.valid)

            let contentYSpy = createTemporaryObject(signalSpyComponent, root,
                { target: verticalFlickable, signalName: "contentYChanged" })
            verify(contentYSpy)
            verify(contentYSpy.valid)

            // Dragging only horizontally should not result in movement on the Y axis, and vice versa.
            let horizontalDragDistance = horizontal ? innerFlickable.width - 10 : 0
            let verticalDragDistance = horizontal ? 0 : innerFlickable.height - 10
            mouseDrag(innerFlickable, 10, 10, horizontalDragDistance, verticalDragDistance)

            // Wait for it to stop moving.
            if (horizontal) {
                tryCompare(horizontalFlickable, "movingHorizontally", false)
                tryCompare(horizontalFlickable, "flickingHorizontally", false)
            } else {
                tryCompare(verticalFlickable, "movingVertically", false)
                tryCompare(verticalFlickable, "flickingVertically", false)
            }

            // 2 because it should change to true then false.
            compare(movingHorizontallySpy.count, horizontal ? 2 : 0)
            compare(movingVerticallySpy.count, horizontal ? 0 : 2)
            compare(flickingHorizontallySpy.count, horizontal ? 2 : 0)
            compare(flickingVerticallySpy.count, horizontal ? 0 : 2)

            if (horizontal)
                verify(contentXSpy.count > 0)
            else
                compare(contentXSpy.count, 0)

            if (horizontal)
                compare(contentYSpy.count, 0)
            else
                verify(contentYSpy.count > 0)
        }

        function test_negativeDragDistance_data() {
            return [
                { tag: "horizontal", startX: 100, startY: 100, xDistance: -90, yDistance: 0 },
                { tag: "vertical", startX: 100, startY: 100, xDistance: 0, yDistance: -90 }
            ]
        }

        // Tests that dragging to the left or top actually results in intermediate mouse moves.
        function test_negativeDragDistance(data) {
            let mouseArea = createTemporaryObject(mouseAreaComponent, root)
            verify(mouseArea)

            let positionSpy = signalSpyComponent.createObject(mouseArea,
                { target: mouseArea, signalName: "positionChanged" })
            verify(positionSpy)
            verify(positionSpy.valid)

            mouseDrag(mouseArea, data.startX, data.startY, data.xDistance, data.yDistance)
            verify(positionSpy.count > 2, "Expected more than 2 mouse position changes, but only got " + positionSpy.count)
        }
    }
}

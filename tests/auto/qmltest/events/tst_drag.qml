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
        property double posX: 0
        property double posY: 0

        function reset(mouseX, mouseY) {
            posX = mouseX;
            posY = mouseY;
            fakeHandle.x = mouseX;
            fakeHandle.y = mouseY;
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

    TestCase {
        name:"mouserelease"
        when:windowShown

        function test_mouseDrag_data() {
            return [
                { tag: "short", dx: 20, dy: 30 },
                { tag: "long", dx: 70, dy: 60 },
                { tag: "longshort", dx: 70, dy: 20 },
                { tag: "shortlong", dx: 20, dy: 70 }
            ];
        }

        function test_mouseDrag(data) {
            container.x = 0;
            container.y = 0;
            mouseDrag(container, 10, 10, data.dx, data.dy);
            compare(container.x, data.dx - util.dragThreshold - 1);
            compare(container.y, data.dy - util.dragThreshold - 1);
        }

        function test_doSomethingInsteadOfDragging_data() {
            return [
                { tag: "short",            updatePositionWhileDragging: false, dx: 2*util.dragThreshold,   dy: 2*util.dragThreshold },
                { tag: "long",             updatePositionWhileDragging: false, dx: 10*util.dragThreshold,  dy: 10*util.dragThreshold },
                { tag: "nothing_short",    updatePositionWhileDragging: false, dx: 0,                      dy: 2*util.dragThreshold },
                { tag: "long_nothing",     updatePositionWhileDragging: false, dx: 10*util.dragThreshold,  dy: 0 },
                { tag: "short_update",     updatePositionWhileDragging: true, dx: 2*util.dragThreshold,   dy: 2*util.dragThreshold },
                { tag: "long_update",      updatePositionWhileDragging: true, dx: 10*util.dragThreshold,  dy: 10*util.dragThreshold },
                { tag: "nothing_short_up", updatePositionWhileDragging: true, dx: 0,                      dy: 2*util.dragThreshold },
                { tag: "long_nothing_up",  updatePositionWhileDragging: true, dx: 10*util.dragThreshold,  dy: 0 },
            ];
        }

        function test_doSomethingInsteadOfDragging(data) {
            var expectedSpyCountX;
            var expectedSpyCountY;

            if (!data.updatePositionWhileDragging) {
                expectedSpyCountX = data.dx > util.dragThreshold ? 1 : 0;
                expectedSpyCountY = data.dy > util.dragThreshold ? 1 : 0;
            } else {
                expectedSpyCountX = data.dx > util.dragThreshold * 3 ? 3 :
                                        (data.dx > util.dragThreshold ? 1 : 0);
                expectedSpyCountY = data.dy > util.dragThreshold * 3 ? 3 :
                                        (data.dy > util.dragThreshold ? 1 : 0);
            }

            container2.updatePositionWhileDragging = data.updatePositionWhileDragging;
            container2.reset(container2.x + 10, container2.y + 10);
            mouseDrag(container2, container2.x + 10, container2.y + 10, data.dx, data.dy);
            compare(spyX.count, expectedSpyCountX)
            compare(spyY.count, expectedSpyCountY)
        }
    }
}

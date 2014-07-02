/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0

TestCase {
    name: "tst_findChild"

    QtObject {
        id: singleObject
    }

    Item {
        id: oneObjectChildItem

        QtObject {
            id: childObject
            objectName: "childObject"
        }
    }

    Item {
        id: oneItemChildItem

        Item {
            id: childItem
            objectName: "childItem"
        }
    }

    Item {
        id: nestedChildrenItem

        Item {
            id: nestedChildItem0
            objectName: "nestedChildItem0"

            Item {
                id: nestedChildItem1
                objectName: "nestedChildItem1"

                Item {
                    id: nestedChildItem2
                    objectName: "nestedChildItem2"
                }
            }
        }
    }

    property Component duplicateNestedChildItem2Component: Item {
        objectName: "nestedChildItem2"
    }

    function test_findChild() {
        compare(findChild(null, ""), null);
        compare(findChild(undefined, ""), null);
        compare(findChild(singleObject, "doesNotExist"), null);
        compare(findChild(oneObjectChildItem, "childObject"), childObject);
        compare(findChild(oneItemChildItem, "childItem"), childItem);
        compare(findChild(nestedChildrenItem, "nestedChildItem0"), nestedChildItem0);
        compare(findChild(nestedChildrenItem, "nestedChildItem1"), nestedChildItem1);
        compare(findChild(nestedChildrenItem, "nestedChildItem2"), nestedChildItem2);

        // Shouldn't be found, since it's not the first in the list.
        duplicateNestedChildItem2Component.createObject(nestedChildItem1);
        compare(nestedChildItem1.children.length, 2);
        compare(findChild(nestedChildrenItem, "nestedChildItem2"), nestedChildItem2);

        var mostDirectChild = duplicateNestedChildItem2Component.createObject(nestedChildItem0);
        compare(nestedChildItem0.children.length, 2);
        compare(findChild(nestedChildrenItem, "nestedChildItem2"), mostDirectChild);
    }
}

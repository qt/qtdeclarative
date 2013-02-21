/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0

Item {
    id: top

    ListView {
        id: emptylist
        height: 20
        width: 50
    }

    ListView {
        id: viewmanyitems
        model: manyitems
    }

    ListView {
        id: modelchange
        model: firstmodel
        delegate: Text { text: model.name }
    }

    ListView {
        id: modelalter
        model: altermodel
        delegate: Text { text: model.name }
    }

    ListModel { id: emptymodel }
    ListModel { id: manyitems }
    ListModel { id: firstmodel; ListElement { name: "FirstModelElement0" } }
    ListModel { id: secondmodel; ListElement { name: "SecondModelElement0" } ListElement { name: "SecondModelElement1" } }
    ListModel { id: altermodel; ListElement { name: "AlterModelElement0" } ListElement { name: "AlterModelElement1" } }

    TestCase {
        name: "ListView"

        function test_empty() {
            compare(emptylist.count, 0)
            emptylist.model = emptymodel;
            compare(emptylist.count, 0)
        }

        function test_multipleitems_data() {
            return [
                {
                    tag: "10items",
                    numitems: 10
                },
                {
                    tag: "100items",
                    numitems: 100
                },
                {
                    tag: "10000items",
                    numitems: 10000
                }
            ]
        }

        function test_multipleitems(row) {
            var i;
            manyitems.clear();
            compare(manyitems.count, 0)
            for (i = 0; i < row.numitems; ++i) {
                manyitems.append({"name":"Item"+i})
            }
            compare(manyitems.count, row.numitems)
            tryCompare(viewmanyitems.count, row.numitems)
        }

        function test_modelchange() {
            tryCompare(modelchange.count, 1)
            modelchange.currentIndex = 0;
            compare(modelchange.currentItem.text, "FirstModelElement0")
            modelchange.model = secondmodel;
            tryCompare(modelchange.count, 2)
            modelchange.currentIndex = 0;
            compare(modelchange.currentItem.text, "SecondModelElement0")
            modelchange.currentIndex = 1;
            compare(modelchange.currentItem.text, "SecondModelElement1")
        }

        function test_modelaltered() {
            tryCompare(modelalter.count, 2)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            altermodel.append({"name":"AlterModelElement2"})
            tryCompare(modelalter.count, 3)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            modelalter.currentIndex = 2;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.insert(2,{"name":"AlterModelElement1.5"})
            tryCompare(modelalter.count, 4)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            modelalter.currentIndex = 2;
            compare(modelalter.currentItem.text, "AlterModelElement1.5")
            modelalter.currentIndex = 3;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.move(2,1,1);
            tryCompare(modelalter.count, 4)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1.5")
            modelalter.currentIndex = 2;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            modelalter.currentIndex = 3;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.remove(1,2)
            tryCompare(modelalter.count, 2)
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement2")
            altermodel.set(1,{"name":"AlterModelElement1"})
            modelalter.currentIndex = 0;
            compare(modelalter.currentItem.text, "AlterModelElement0")
            modelalter.currentIndex = 1;
            compare(modelalter.currentItem.text, "AlterModelElement1")
            altermodel.clear()
            tryCompare(modelalter.count, 0)
            compare(modelalter.currentItem, null)
        }
    }
}

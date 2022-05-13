// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12
import Qt.labs.qmlmodels 1.0

TableView {
    width: 200; height: 200
    model: TestModel {
        id: testModel

        // This is silly: in real life, store the birthdate instead of the age,
        // and let the delegate calculate the age, so it won't need updating
        function happyBirthday(dude) {
            var row = -1;
            for (var r = 0; row < 0 && r < testModel.rowCount; ++r)
                if (testModel.data(testModel.index(r, 0), "display") === dude)
                    row = r;
            var index = testModel.index(row, 1)
            testModel.setData(index, "display", testModel.data(index, "display") + 1)
        }
    }
    delegate: Text {
        id: textItem
        text: model.display
        TapHandler {
            onTapped: testModel.happyBirthday(testModel.data(testModel.index(row, 0), "display"))
        }
    }
}

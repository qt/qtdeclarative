// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.12
import QtQml.Models 2.12
import Qt.labs.qmlmodels 1.0

ListView {
    width: 400
    height: 400
    model: 8

    delegate: DelegateChooser {
        DelegateChoice {
            index: 0
            delegate: Item {
                property string choiceType: "1"
                width: parent.width
                height: 50
            }
        }
        DelegateChoice {
            index: 1
            delegate: Item {
                property string choiceType: "2"
                width: parent.width
                height: 50
            }
        }
        DelegateChoice {
            index: 2
            delegate: Item {
                property string choiceType: "3"
                width: parent.width
                height: 50
            }
        }
        DelegateChoice {
            index: 5
            delegate: Item {
                property string choiceType: "3"
                width: parent.width
                height: 50
            }
        }
        DelegateChoice {
            delegate: Item {
                property string choiceType: "4"
                width: parent.width
                height: 50
            }
        }
    }
}

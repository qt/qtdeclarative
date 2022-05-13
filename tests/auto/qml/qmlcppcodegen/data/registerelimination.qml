// Copyright (C) 2021 The Qt Company Ltd.

import QtQuick

Rectangle {
    id: page
    height: 480;

    property int input: 10
    property int output: ball.y

    onInputChanged: ball.y = input

    Rectangle {
        id: ball

        onYChanged: {
            if (y <= 0) {
                y = page.height - 21;
            } else if (y >= page.height - 20) {
                y = 0;
            }
        }
    }
}

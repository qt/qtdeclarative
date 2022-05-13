// Copyright (C) 2021 The Qt Company Ltd.

import QtQml

QtObject {
    id: self
    onObjectNameChanged: {
        try {
            Qt.createQmlObject("1", self, 'CustomObject');
        } catch(err) {
        }
    }
}


// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import RenderNode 1.0

Item {
    width: 320
    height: 480

    Item { x: 10;   y: 10;  width: 10;  height: 10;
        StateRecorder { x: 10; y: 10; objectName: "no-clip; no-rotation"; }
    }

    Item { x: 10;   y: 10;  width: 10;  height: 10; clip: true
        StateRecorder { x: 10; y: 10; objectName: "parent-clip; no-rotation"; }
    }

    Item { x: 10;   y: 10;  width: 10;  height: 10;
        StateRecorder { x: 10; y: 10; objectName: "self-clip; no-rotation"; clip: true }
    }


    Item { x: 10;   y: 10;  width: 10;  height: 10; rotation: 90
        StateRecorder { x: 10; y: 10; objectName: "no-clip; parent-rotation"; }
    }

    Item { x: 10;   y: 10;  width: 10;  height: 10; clip: true; rotation: 90
        StateRecorder { x: 10; y: 10; objectName: "parent-clip; parent-rotation"; }
    }

    Item { x: 10;   y: 10;  width: 10;  height: 10; rotation: 90
        StateRecorder { x: 10; y: 10; objectName: "self-clip; parent-rotation"; clip: true }
    }


    Item { x: 10;   y: 10;  width: 10;  height: 10;
        StateRecorder { x: 10; y: 10; objectName: "no-clip; self-rotation"; rotation: 90 }
    }

    Item { x: 10;   y: 10;  width: 10;  height: 10; clip: true;
        StateRecorder { x: 10; y: 10; objectName: "parent-clip; self-rotation"; rotation: 90}
    }

    Item { x: 10;   y: 10;  width: 10;  height: 10;
        StateRecorder { x: 10; y: 10; objectName: "self-clip; self-rotation"; clip: true; rotation: 90 }
    }

}

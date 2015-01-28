/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    Repeater {
        model: 100
        delegate: Image {
            id: img
            Component.onCompleted: {
                sys.acc = sys.acc + 1;
                ip.take(img);
            }
            Component.onDestruction: sys.acc = sys.acc - 1;

            //Test uses the recycling case because it's most realistic
            //Attempts by ItemParticle to delete the delegate should lead to a segfault
            ItemParticle.onDetached: ip.take(img);

            source: "../../shared/star.png"
        }
    }

    ParticleSystem {
        id: sys
        objectName: "system"
        anchors.fill: parent
        property int acc: 0

        ItemParticle {
            id: ip
        }

        Emitter{
            //0,0 position
            size: 32
            emitRate: 1000
            lifeSpan: 100
        }
    }
}

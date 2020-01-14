/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import QtQuick.Particles 2.0

Rectangle {
    id: testTakeGive
    color: "black"
    width: 320
    height: 320

    function takeItems()
    {
        for(var i = 0; i < imgList.count; i++) ip.take(imgList.itemAt(i));
    }
    function giveItems()
    {
        for(var i = 0; i < imgList.count; i++) ip.give(imgList.itemAt(i));
    }

    Repeater {
        id: imgList
        model: 100
        delegate: Image {
            ItemParticle.onAttached: sys.acc = sys.acc + 1
            ItemParticle.onDetached: sys.acc = sys.acc - 1;
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
            lifeSpan: Emitter.InfiniteLife
        }
    }
}

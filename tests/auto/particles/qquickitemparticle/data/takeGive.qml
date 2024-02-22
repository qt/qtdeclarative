// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
            objectName: "emitter"
            //0,0 position
            size: 32
            emitRate: 0
            lifeSpan: Emitter.InfiniteLife
        }
    }
}

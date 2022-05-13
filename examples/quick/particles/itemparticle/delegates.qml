// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root;
    width: 360
    height: 600
    color: "black"

    function newPithySaying() {
        switch (Math.floor(Math.random()*16)) {
            case 0: return "Hello World";
            case 1: return "G'day Mate";
            case 2: return "Code Less";
            case 3: return "Create More";
            case 4: return "Deploy Everywhere";
            case 5: return "Qt Meta-object Language";
            case 6: return "Qt Magic Language";
            case 7: return "Fluid UIs";
            case 8: return "Touchable";
            case 9: return "How's it going?";
            case 10: return "Do you like text?";
            case 11: return "Enjoy!";
            case 12: return "ERROR: Out of pith";
            case 13: return "Punctuation Failure";
            case 14: return "I can go faster";
            case 15: return "I can go slower";
            default: return "OMGWTFBBQ";
        }
    }

    ParticleSystem {
        anchors.fill: parent
        id: syssy
        MouseArea {
            anchors.fill: parent
            onClicked: syssy.running = !syssy.running
        }
        Emitter {
            anchors.centerIn: parent
            emitRate: 1
            lifeSpan: 4800
            lifeSpanVariation: 1600
            velocity: AngleDirection {angleVariation: 360; magnitude: 40; magnitudeVariation: 20}
        }
        ItemParticle {
            delegate: Text {
                text: root.newPithySaying();
                color: "white"
                font.pixelSize: 18
                font.bold: true
            }
        }
    }
}

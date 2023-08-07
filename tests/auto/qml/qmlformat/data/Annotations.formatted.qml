#!/she-bang
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//![2]
import QtQuick 2.0
//![2]
import QtCharts 2.0

@Pippo {
    atg1: 3
}
@Annotation2 {
}
Item {
    @Annotate {
    }
    anchors.fill: parent
    @AnnotateMore {
        property int x: 5
    }
    @AnnotateALot {
    }
    property variant othersSlice: 0

    //![1]
    ChartView {
        id: chart
        title: "Top-5 car brand shares in Finland"
        anchors.fill: parent
        legend.alignment: Qt.AlignBottom
        antialiasing: true
        @ExtraAnnotation {
            signal pippo
        }
        PieSeries {
            @IdAnnotation {
                des: "pippo"
            }
            id: pieSeries
            @Maximum {
                nTied: 0
            }
            PieSlice {
                label: "Volkswagen"
                value: 13.5
            }
            PieSlice {
                label: "Toyota"
                value: 10.9
            }
            PieSlice {
                label: "Ford"
                value: 8.6
            }
            PieSlice {
                label: "Skoda"
                value: 8.2
            }
            PieSlice {
                label: "Volvo"
                value: 6.8
            }
        }
        @SignalAnnotation {
            implicit: true
        }
        signal pippo2 // nice signal
        @MethodAnnotation {
            important: true
        }
        function foo(x) {
            return 42;
        }
        @BindingAnn {
            bType: 1
        }
        val: 34
        @BindingAnn {
            bType: 2
        }
        val2: Item {
        }
        @BindingAnn {
            bType: 3
        }
        val3: [
            Item {
            }
        ]
        @BindingAnn {
            bType: 4
        }
        Animation on val {
            duration: 34
        }
    }
    @SuperComplete {
        binding: late
    }
    Component.onCompleted: {
        // You can also manipulate slices dynamically, like append a slice or set a slice exploded
        othersSlice = pieSeries.append("Others", 52.0);
        pieSeries.find("Volkswagen").exploded = true;
    }
    //![1]
}

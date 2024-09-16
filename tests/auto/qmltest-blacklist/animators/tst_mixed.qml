// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testCase
        name: "animators-mixed"
        when: !rootAnimation.running
        function test_endresult() {
            compare(box.rootStart, 2);
            compare(box.rootEnd, 2);

            compare(parallelWithOneSequential.before, 4);
            compare(parallelWithOneSequential.scaleUpdates, 4);

            compare(parallelWithTwoSequentialNormalEndsLast.beforeAnimator, 4);
            compare(parallelWithTwoSequentialNormalEndsLast.scaleUpdates, 4);
            compare(parallelWithTwoSequentialNormalEndsLast.afterAnimator, 4);
            compare(parallelWithTwoSequentialNormalEndsLast.beforePause, 4);
            compare(parallelWithTwoSequentialNormalEndsLast.afterPause, 4);

            compare(parallelWithTwoSequentialNormalEndsFirst.beforeAnimator, 4);
            compare(parallelWithTwoSequentialNormalEndsFirst.scaleUpdates, 4);
            compare(parallelWithTwoSequentialNormalEndsFirst.afterAnimator, 4);
            compare(parallelWithTwoSequentialNormalEndsFirst.beforePause, 4);
            compare(parallelWithTwoSequentialNormalEndsFirst.afterPause, 4);

        }
    }

    Box {
        id: box

        property int rootStart : 0
        property int rootEnd : 0;

        SequentialAnimation {
            id: rootAnimation

            running: true
            loops: 2

            ScriptAction { script: box.rootStart++; }

            ParallelAnimation {
                id: parallelWithOneSequential
                property int before : 0;
                property int scaleUpdates : 0;
                loops: 2
                SequentialAnimation {
                    ScriptAction { script: {
                            parallelWithOneSequential.before++;
                            box.scale = 1;
                            box.scaleChangeCounter = 0;
                        }
                    }
                    ScaleAnimator { target: box; from: 1; to: 2; duration: 100; }
                    ScriptAction { script: {
                            parallelWithOneSequential.scaleUpdates += box.scaleChangeCounter;
                        }
                    }
                }
            }

            ParallelAnimation {
                id: parallelWithTwoSequentialNormalEndsLast
                property int beforeAnimator : 0;
                property int scaleUpdates : 0;
                property int afterAnimator : 0;
                property int beforePause : 0;
                property int afterPause : 0;
                loops: 2
                SequentialAnimation {
                    ScriptAction { script: {
                            parallelWithTwoSequentialNormalEndsLast.beforeAnimator++;
                            box.scale = 1;
                            box.scaleChangeCounter = 0;
                        }
                    }
                    ScaleAnimator { target: box; from: 1; to: 2; duration: 100; }
                    ScriptAction { script: {
                            parallelWithTwoSequentialNormalEndsLast.scaleUpdates += box.scaleChangeCounter;
                            parallelWithTwoSequentialNormalEndsLast.afterAnimator++;
                        }
                    }
                }
                SequentialAnimation {
                    ScriptAction { script: {
                            parallelWithTwoSequentialNormalEndsLast.beforePause++
                        }
                    }
                    PauseAnimation { duration: 200 }
                    ScriptAction { script: {
                            parallelWithTwoSequentialNormalEndsLast.afterPause++
                        }
                    }
                }
            }

            ParallelAnimation {
                id: parallelWithTwoSequentialNormalEndsFirst
                property int beforeAnimator : 0;
                property int scaleUpdates : 0;
                property int afterAnimator : 0;
                property int beforePause : 0;
                property int afterPause : 0;
                loops: 2
                SequentialAnimation {
                    ScriptAction { script: {
                            parallelWithTwoSequentialNormalEndsFirst.beforeAnimator++;
                            box.scale = 1;
                            box.scaleChangeCounter = 0;
                        }
                    }
                    ScaleAnimator { target: box; from: 1; to: 2; duration: 200; }
                    ScriptAction { script: {
                            parallelWithTwoSequentialNormalEndsFirst.scaleUpdates += box.scaleChangeCounter;
                            parallelWithTwoSequentialNormalEndsFirst.afterAnimator++;
                        }
                    }
                }
                SequentialAnimation {
                    ScriptAction { script: parallelWithTwoSequentialNormalEndsFirst.beforePause++ }
                    PauseAnimation { duration: 100 }
                    ScriptAction { script: parallelWithTwoSequentialNormalEndsFirst.afterPause++ }
                }
            }

            ScriptAction { script: box.rootEnd++; }
        }

    }

}

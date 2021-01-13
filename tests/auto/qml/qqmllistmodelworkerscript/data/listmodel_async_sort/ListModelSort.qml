/* Copyright 2020 Esri
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

import QtQml 2.13
import QtQml.Models 2.13

import QtQuick 2.13
import QtQuick.Layouts 1.12

import "ListModelSort.mjs" as ListModelSort

Item {
    Component.onCompleted: {
        fillModel(namesModel, "A, D, E, B, C, A");
    }


    function verify() {
        let expected = ["A", "A", "B", "C", "D", "E"]
        let sorted = listView.count === 6
        for (let i = 0; i<listView.count;++i) {
            sorted = sorted && (listView.itemAtIndex(i).name === expected[i])
        }
        return sorted
    }

    function doSort() {
        sortAsync(namesModel, "name", false, false)
    }


    function random(n) {
        return Math.round(Math.random(new Date().valueOf()) * n);
    }

    function fillModel(model, values) {
        var names = values.split(",").map(name => name.trim()).filter(name => name > "");


        model.clear();

        for (const name of names) {
            model.append({
                             name: name,
                             number: random(names.length)
                         });
        }
    }

    //--------------------------------------------------------------------------

    ListModel {
        id: namesModel

        objectName: "namesModel"
    }

    //--------------------------------------------------------------------------
    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }

        ListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            model: namesModel
            clip: true
            spacing: 5

            delegate: Text {
                required property string name
                required property int number
                width: ListView.view.width
                text: "%1 (%2)".arg(name).arg(number)
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }

            add: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
                NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }
            }

            addDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 100 }
            }

            move: Transition {
                NumberAnimation { properties: "x,y"; duration: 100 }
            }

            moveDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 100 }
            }

            displaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 40; easing.type: Easing.OutBounce }
            }

            populate: Transition {
                NumberAnimation { properties: "x,y"; duration: 100 }
            }

            remove: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; to: 0; duration: 100 }
                    NumberAnimation { properties: "x,y"; to: 100; duration: 100 }
                }
            }

            removeDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 100 }
            }
        }
        }


    function sortSync(model, sortKey, descending, useMove) {

        ListModelSort.sort(model, sortKey, descending, useMove);
    }

    //--------------------------------------------------------------------------

    function sortAsync(model, sortKey, descending, useMove) {

        var params = {
            debug: true,
            model: model,
            sortKey: sortKey,
            descending: descending,
            useMove: useMove
        };


        sortWorker.sendMessage(params);
    }

    WorkerScript {
        id: sortWorker

        source: "ListModelSortWorker.mjs"
    }

    //--------------------------------------------------------------------------

}


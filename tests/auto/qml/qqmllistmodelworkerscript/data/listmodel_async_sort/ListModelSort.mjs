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

//------------------------------------------------------------------------------

export const OrderAscending = 0
export const OrderDescending = 1

//------------------------------------------------------------------------------

export function sort(model, sortKey, descending, useMove) {

    _sort(model, sortKey, descending, useMove);
}

//------------------------------------------------------------------------------

function _sort(model, sortKey, descending, useMove) {
    var lessThan =  descending ? 1 : -1;
    var greaterThan = -lessThan;

    var indices = [...new Array(model.count).keys()];

    function getKeyValue(index) {
        return model.get(index)[sortKey];
    }

    function compareKey(index1, index2) {
        var value1 = getKeyValue(index1);
        var value2 = getKeyValue(index2);

        return (value1 < value2)
                ? lessThan
                : (value1 > value2)
                  ? greaterThan
                  : 0;
    }

    indices.sort(compareKey);


    function compareFrom(i1, i2) {
        return (i1.from < i2.from)
                ? -1
                : (i1.from > i2.from)
                  ? 1
                  : 0;
    }

    var swapIndices = indices.map((e, i) => { return { from: e, to: i }}).sort(compareFrom).map(e => e.to);

    function clone(o) {
        return JSON.parse(JSON.stringify(o));
    }

    function cloneSwap(a, b) {
        var o = clone(model.get(a));
        model.set(a, model.get(b));
        model.set(b, o);
    }

    function moveSwap(a, b) {
        if (a < b) {
            model.move(a, b, 1);
            model.move(b - 1, a, 1);
        }
        else if (a > b) {
            model.move(b, a, 1);
            model.move(a - 1, b, 1);
        }
    }


    var swap = useMove ? moveSwap : cloneSwap;
    var swapCount = 0;


    for (var iFrom = 0; iFrom < swapIndices.length; iFrom++) {
        var iTo = swapIndices[iFrom];

        if (iFrom === iTo) {
            continue;
        }

        do {
            swap(iFrom, iTo);

            var t = swapIndices[iFrom];
            swapIndices[iFrom] = swapIndices[iTo]
            swapIndices[iTo] = t;

            iTo = swapIndices[iFrom];

            swapCount++;
        }
        while (iFrom !== iTo);
    }

}

//------------------------------------------------------------------------------




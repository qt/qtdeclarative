// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.4
import QtTest 1.1

Item {
    id: root;
    width: 400
    height: 400

    TestCase {
        id: testCase
        name: "item-grabber"
        when: imageOnDisk.ready && imageOnDiskSmall.ready

        function test_endresult_disk() {
            if (Qt.platform.pluginName === "minimal")
                skip("grabImage does not work on offscreen/minimal platforms");

            var image = grabImage(root);

            // imageOnDisk at (0, 0) - (100x100)
            compare(imageOnDisk.width, 100);
            compare(imageOnDisk.height, 100);
            compare(image.pixel(0, 0), Qt.rgba(1, 0, 0, 1));
            compare(image.pixel(99, 99), Qt.rgba(0, 0, 1, 1));

            // imageOnDiskSmall at (100, 0) - 50x50
            compare(imageOnDiskSmall.width, 50);
            compare(imageOnDiskSmall.height, 50);
            verify(image.pixel(100, 0) === Qt.rgba(1, 0, 0, 1));
            verify(image.pixel(149, 49) === Qt.rgba(0, 0, 1, 1));
        }

        function test_endresult_cache_data() {
            return [
                { cache: true, sourceSize: Qt.size(-1, -1), fillMode: Image.Stretch },
                { cache: true, sourceSize: Qt.size(-1, -1), fillMode: Image.PreserveAspectFit },
                { cache: true, sourceSize: Qt.size(-1, -1), fillMode: Image.PreserveAspectCrop },
                { cache: true, sourceSize: Qt.size(10, 10), fillMode: Image.Stretch },
                { cache: true, sourceSize: Qt.size(10, 10), fillMode: Image.PreserveAspectFit },
                { cache: true, sourceSize: Qt.size(10, 10), fillMode: Image.PreserveAspectCrop },
                { cache: false, sourceSize: Qt.size(-1, -1), fillMode: Image.Stretch },
                { cache: false, sourceSize: Qt.size(-1, -1), fillMode: Image.PreserveAspectFit },
                { cache: false, sourceSize: Qt.size(-1, -1), fillMode: Image.PreserveAspectCrop },
                { cache: false, sourceSize: Qt.size(10, 10), fillMode: Image.Stretch },
                { cache: false, sourceSize: Qt.size(10, 10), fillMode: Image.PreserveAspectFit },
                { cache: false, sourceSize: Qt.size(10, 10), fillMode: Image.PreserveAspectCrop },
            ];
        }

        function test_endresult_cache(data) {
            if (Qt.platform.pluginName === "minimal")
                skip("grabImage does not work on offscreen/minimal platforms");

            imageInCache.cache = data.cache;
            imageInCache.sourceSize = data.sourceSize;
            imageInCache.fillMode = data.fillMode;
            imageInCacheSmall.cache = data.cache;
            imageInCacheSmall.sourceSize = data.sourceSize;
            imageInCacheSmall.fillMode = data.fillMode;

            box.grabToImage(imageInCache.handleGrab);
            box.grabToImage(imageInCacheSmall.handleGrab, Qt.size(50, 50));

            tryCompare(imageInCache, "ready", true);
            tryCompare(imageInCacheSmall, "ready", true);

            var image = grabImage(root);

            // imageInCache at (0, 100) - 100x100
            compare(imageInCache.width, 100);
            compare(imageInCache.height, 100);
            compare(image.pixel(0, 100), Qt.rgba(1, 0, 0, 1));
            compare(image.pixel(99, 199), Qt.rgba(0, 0, 1, 1));

            // imageInCacheSmall at (100, 100) - 50x50
            compare(imageInCacheSmall.width, 50);
            compare(imageInCacheSmall.height, 50);
            verify(image.pixel(100, 100) === Qt.rgba(1, 0, 0, 1));
            verify(image.pixel(149, 149) === Qt.rgba(0, 0, 1, 1));

            // After all that has been going on, it should only have been called that one time..
            compare(imageOnDisk.callCount, 1);
        }

        onWindowShownChanged: {
            box.grabToImage(imageOnDisk.handleGrab);
            box.grabToImage(imageOnDiskSmall.handleGrab, Qt.size(50, 50));
        }

    }

    Rectangle {
        id: box
        width: 100
        height: 100
        color: "red";

        visible: false

        Rectangle {
            anchors.bottom: parent.bottom;
            anchors.right: parent.right;
            width: 10
            height: 10
            color: "blue";
        }
    }

    Image {
        id: imageOnDisk
        x: 0
        y: 0
        property int callCount: 0;
        property bool ready: false;
        function handleGrab(result) {
            if (!result.saveToFile("image.png"))
                print("Error: Failed to save image to disk...");
            source = "image.png";
            ready = true;
            ++callCount;
        }
    }

    Image {
        id: imageOnDiskSmall
        x: 100
        y: 0
        property bool ready: false;
        function handleGrab(result) {
            if (!result.saveToFile("image_small.png"))
                print("Error: Failed to save image to disk...");
            source = "image_small.png";
            ready = true;
        }
    }

    Image {
        id: imageInCache
        x: 0
        y: 100
        property bool ready: false;
        function handleGrab(result) {
            source = result.url;
            ready = true;
        }
    }

    Image {
        id: imageInCacheSmall
        x: 100
        y: 100
        property bool ready: false;
        function handleGrab(result) {
            source = result.url;
            ready = true;
        }
    }
}

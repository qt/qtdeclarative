import QtQuick

RangeMover {
    property double zoomer: 0
    function updateZoomer() { zoomer = rangeWidth }
    onRangeWidthChanged: updateZoomer()
}

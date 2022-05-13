// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TEXTBALLOON_H
#define TEXTBALLOON_H

#include <QtQuick>

//! [0]
class TextBalloon : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(bool rightAligned READ isRightAligned WRITE setRightAligned NOTIFY rightAlignedChanged)
    QML_ELEMENT

    public:
        TextBalloon(QQuickItem *parent = nullptr);
        void paint(QPainter *painter) override;

        bool isRightAligned() const;
        void setRightAligned(bool rightAligned);

    private:
        bool rightAligned;

    signals:
        void rightAlignedChanged();
};
//! [0]

#endif

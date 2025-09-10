// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMSLIDER_H
#define QQUICKSTYLEITEMSLIDER_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickslider_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemSlider : public QQuickStyleItem
{
    Q_OBJECT

    Q_PROPERTY(SubControl subControl MEMBER m_subControl)

    QML_NAMED_ELEMENT(Slider)

public:
    enum SubControl {
        Groove = 1,
        Handle,
    };
    Q_ENUM(SubControl)

    QFont styleFont(QQuickItem *control) const override;

protected:
    void connectToControl() const override;
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    void initStyleOption(QStyleOptionSlider &styleOption) const;

private:
   SubControl m_subControl = Groove;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMSLIDER_H

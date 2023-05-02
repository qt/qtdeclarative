// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMSCROLLBAR_H
#define QQUICKSTYLEITEMSCROLLBAR_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickscrollbar_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemScrollBar : public QQuickStyleItem
{
    Q_OBJECT

    Q_PROPERTY(SubControl subControl MEMBER m_subControl)

    QML_NAMED_ELEMENT(ScrollBar)

public:
    enum SubControl {
        Groove = 1,
        Handle,
        AddLine,
        SubLine
    };
    Q_ENUM(SubControl)

    explicit QQuickStyleItemScrollBar(QQuickItem *parent = nullptr);

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

#endif // QQUICKSTYLEITEMSCROLLBAR_H

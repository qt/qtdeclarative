// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTYLEITEMSPINBOX_H
#define QQUICKSTYLEITEMSPINBOX_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickspinbox_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemSpinBox : public QQuickStyleItem
{
    Q_OBJECT

    Q_PROPERTY(SubControl subControl MEMBER m_subControl)

    QML_NAMED_ELEMENT(SpinBox)

public:
    enum SubControl {
        Frame = 1,
        Up,
        Down,
    };
    Q_ENUM(SubControl)

    QFont styleFont(QQuickItem *control) const override;

protected:
    void connectToControl() const override;
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    void initStyleOption(QStyleOptionSpinBox &styleOption) const;

private:
   SubControl m_subControl = Frame;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMSPINBOX_H

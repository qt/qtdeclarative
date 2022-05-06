/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQUICKSTYLEITEMSPINBOX_H
#define QQUICKSTYLEITEMSPINBOX_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickspinbox_p.h>

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

#endif // QQUICKSTYLEITEMSPINBOX_H

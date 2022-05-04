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

#ifndef QQUICKSTYLEITEMGROUPBOX_H
#define QQUICKSTYLEITEMGROUPBOX_H

#include "qquickstyleitem.h"
#include <QtQuickTemplates2/private/qquickgroupbox_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyleItemGroupBox : public QQuickStyleItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickStyleMargins groupBoxPadding READ groupBoxPadding NOTIFY groupBoxPaddingChanged)
    Q_PROPERTY(QPointF labelPos READ labelPos NOTIFY labelPosChanged)
    QML_NAMED_ELEMENT(GroupBox)

public:
    QQuickStyleMargins groupBoxPadding() const;
    QPointF labelPos() const;
    QFont styleFont(QQuickItem *control) const override;

signals:
    void groupBoxPaddingChanged();
    void labelPosChanged();

protected:
    void paintEvent(QPainter *painter) const override;
    StyleItemGeometry calculateGeometry() override;

private:
    QQuickStyleMargins m_groupBoxPadding;
    QPointF m_labelPos;

    void initStyleOption(QStyleOptionGroupBox &styleOption) const;
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEITEMGROUPBOX_H

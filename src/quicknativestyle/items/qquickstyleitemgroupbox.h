// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

Q_SIGNALS:
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

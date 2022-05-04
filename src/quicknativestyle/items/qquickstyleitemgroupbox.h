/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <QtCore/QWeakPointer>
#include <QtQuick/QQuickPaintedItem>

namespace QmlJSDebugger {
namespace QtQuick2 {

class Highlight : public QQuickPaintedItem
{
    Q_OBJECT

public:
    Highlight(QQuickItem *parent) : QQuickPaintedItem(parent) {}
    Highlight(QQuickItem *item, QQuickItem *parent);

    void setItem(QQuickItem *item);

private slots:
    void adjust();

private:
    QWeakPointer<QQuickItem> m_item;
};

/**
 * A highlight suitable for indicating selection.
 */
class SelectionHighlight : public Highlight
{
public:
    SelectionHighlight(QQuickItem *item, QQuickItem *parent)
        : Highlight(item, parent)
    {}

    void paint(QPainter *painter);
};

/**
 * A highlight suitable for indicating hover.
 */
class HoverHighlight : public Highlight
{
public:
    HoverHighlight(QQuickItem *parent)
        : Highlight(parent)
    {
        setZ(1); // hover highlight on top of selection highlight
    }

    void paint(QPainter *painter);
};

} // namespace QtQuick2
} // namespace QmlJSDebugger

#endif // HIGHLIGHT_H

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <QtCore/QPointer>
#include <QtCore/QPointF>
#include <QtGui/QTransform>
#include <QtQuick/QQuickPaintedItem>

QT_BEGIN_NAMESPACE

namespace QmlJSDebugger {

class Highlight : public QQuickPaintedItem
{
    Q_OBJECT

public:
    Highlight(QQuickItem *parent);
    Highlight(QQuickItem *item, QQuickItem *parent);

    void setItem(QQuickItem *item);
    QQuickItem *item() {return m_item;}

protected:
    QTransform transform() {return m_transform;}

private:
    void initRenderDetails();
    void adjust();

private:
    QPointer<QQuickItem> m_item;
    QTransform m_transform;
};

/**
 * A highlight suitable for indicating selection.
 */
class SelectionHighlight : public Highlight
{
    Q_OBJECT

public:
    SelectionHighlight(const QString &name, QQuickItem *item, QQuickItem *parent);
    void paint(QPainter *painter) override;
    void showName(const QPointF &displayPoint);

private:
    QPointF m_displayPoint;
    QString m_name;
    bool m_nameDisplayActive;

    void disableNameDisplay();
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

    void paint(QPainter *painter) override;
};

} // namespace QmlJSDebugger

QT_END_NAMESPACE

#endif // HIGHLIGHT_H

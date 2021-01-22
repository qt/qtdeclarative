/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
**
**
****************************************************************************/

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

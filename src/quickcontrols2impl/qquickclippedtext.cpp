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

#include "qquickclippedtext_p.h"

#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickClippedText::QQuickClippedText(QQuickItem *parent)
    : QQuickText(parent)
{
}

qreal QQuickClippedText::clipX() const
{
    return m_clipX;
}

void QQuickClippedText::setClipX(qreal x)
{
    if (qFuzzyCompare(x, m_clipX))
        return;

    m_clipX = x;
    markClipDirty();
}

qreal QQuickClippedText::clipY() const
{
    return m_clipY;
}

void QQuickClippedText::setClipY(qreal y)
{
    if (qFuzzyCompare(y, m_clipY))
        return;

    m_clipY = y;
    markClipDirty();
}

qreal QQuickClippedText::clipWidth() const
{
    return m_clipWidth ? m_clipWidth : width();
}

void QQuickClippedText::setClipWidth(qreal width)
{
    m_hasClipWidth = true;
    if (qFuzzyCompare(width, m_clipWidth))
        return;

    m_clipWidth = width;
    markClipDirty();
}

qreal QQuickClippedText::clipHeight() const
{
    return m_clipHeight ? m_clipHeight : height();
}

void QQuickClippedText::setClipHeight(qreal height)
{
    m_hasClipHeight = true;
    if (qFuzzyCompare(height, m_clipHeight))
        return;

    m_clipHeight = height;
    markClipDirty();
}

QRectF QQuickClippedText::clipRect() const
{
    return QRectF(clipX(), clipY(), clipWidth(), clipHeight());
}

void QQuickClippedText::markClipDirty()
{
    QQuickItemPrivate::get(this)->dirty(QQuickItemPrivate::Size);
}

QT_END_NAMESPACE

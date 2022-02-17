/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickinputmethod_p.h"

#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype InputMethod
    \inqmlmodule QtQuick.

    \brief Provides access to \l QInputMethod for QML applications.

    The InputMethod singleton allows access to application's \l QInputMethod object
    and all its properties and slots. See the \l QInputMethod documentation for
    further details.
*/

QQuickInputMethod::QQuickInputMethod(QObject *parent) : QObject(parent)
{
    QInputMethod *inputMethod = QGuiApplication::inputMethod();
    connect(inputMethod, &QInputMethod::anchorRectangleChanged, this,
            &QQuickInputMethod::anchorRectangleChanged);
    connect(inputMethod, &QInputMethod::animatingChanged, this,
            &QQuickInputMethod::animatingChanged);
    connect(inputMethod, &QInputMethod::cursorRectangleChanged, this,
            &QQuickInputMethod::cursorRectangleChanged);
    connect(inputMethod, &QInputMethod::inputDirectionChanged, this,
            &QQuickInputMethod::inputDirectionChanged);
    connect(inputMethod, &QInputMethod::inputItemClipRectangleChanged, this,
            &QQuickInputMethod::inputItemClipRectangleChanged);
    connect(inputMethod, &QInputMethod::keyboardRectangleChanged, this,
            &QQuickInputMethod::keyboardRectangleChanged);
    connect(inputMethod, &QInputMethod::localeChanged, this, &QQuickInputMethod::localeChanged);
    connect(inputMethod, &QInputMethod::visibleChanged, this, &QQuickInputMethod::visibleChanged);
}

void QQuickInputMethod::commit()
{
    QGuiApplication::inputMethod()->commit();
}
void QQuickInputMethod::hide()
{
    QGuiApplication::inputMethod()->hide();
}
void QQuickInputMethod::invokeAction(QInputMethod::Action a, int cursorPosition)
{
    QGuiApplication::inputMethod()->invokeAction(a, cursorPosition);
}
void QQuickInputMethod::reset()
{
    QGuiApplication::inputMethod()->reset();
}
void QQuickInputMethod::show()
{
    QGuiApplication::inputMethod()->show();
}
void QQuickInputMethod::update(Qt::InputMethodQueries queries)
{
    QGuiApplication::inputMethod()->update(queries);
}

QRectF QQuickInputMethod::anchorRectangle() const
{
    return QGuiApplication::inputMethod()->cursorRectangle();
}
QRectF QQuickInputMethod::cursorRectangle() const
{
    return QGuiApplication::inputMethod()->cursorRectangle();
}
Qt::LayoutDirection QQuickInputMethod::inputDirection() const
{
    return QGuiApplication::inputMethod()->inputDirection();
}
QRectF QQuickInputMethod::inputItemClipRectangle() const
{
    return QGuiApplication::inputMethod()->inputItemClipRectangle();
}

QRectF QQuickInputMethod::inputItemRectangle() const
{
    return QGuiApplication::inputMethod()->inputItemRectangle();
}
void QQuickInputMethod::setInputItemRectangle(const QRectF &rect)
{
    QGuiApplication::inputMethod()->setInputItemRectangle(rect);
}

QTransform QQuickInputMethod::inputItemTransform() const
{
    return QGuiApplication::inputMethod()->inputItemTransform();
}
void QQuickInputMethod::setInputItemTransform(const QTransform &transform)
{
    QGuiApplication::inputMethod()->setInputItemTransform(transform);
}

bool QQuickInputMethod::isAnimating() const
{
    return QGuiApplication::inputMethod()->isAnimating();
}

bool QQuickInputMethod::isVisible() const
{
    return QGuiApplication::inputMethod()->isVisible();
}
void QQuickInputMethod::setVisible(bool visible)
{
    QGuiApplication::inputMethod()->setVisible(visible);
}

QRectF QQuickInputMethod::keyboardRectangle() const
{
    return QGuiApplication::inputMethod()->keyboardRectangle();
}
QLocale QQuickInputMethod::locale() const
{
    return QGuiApplication::inputMethod()->locale();
}

QT_END_NAMESPACE

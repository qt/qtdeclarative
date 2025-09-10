// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickinputmethod_p.h"

#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype InputMethod
    \inqmlmodule QtQuick

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

#include "moc_qquickinputmethod_p.cpp"

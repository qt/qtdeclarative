/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickfocusscope_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype FocusScope
    \instantiates QQuickFocusScope
    \inqmlmodule QtQuick
    \ingroup qtquick-input

    \brief Explicitly creates a focus scope.
    \inherits Item

    Focus scopes assist in keyboard focus handling when building reusable QML
    components.  All the details are covered in the
    \l {Keyboard Focus in Qt Quick}{keyboard focus documentation}.

    \sa {Qt Quick Examples - Key Interaction}
*/
QQuickFocusScope::QQuickFocusScope(QQuickItem *parent)
: QQuickItem(parent)
{
    setFlag(ItemIsFocusScope);
}

QQuickFocusScope::~QQuickFocusScope()
{
}

QT_END_NAMESPACE

#include "moc_qquickfocusscope_p.cpp"

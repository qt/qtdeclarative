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

#include "qquickplaceholdertext_p.h"

#include <QtQuick/private/qquicktext_p_p.h>
#include <QtQuick/private/qquicktextinput_p_p.h>
#include <QtQuick/private/qquicktextedit_p_p.h>

QT_BEGIN_NAMESPACE

QQuickPlaceholderText::QQuickPlaceholderText(QQuickItem *parent) : QQuickText(parent)
{
}

void QQuickPlaceholderText::componentComplete()
{
    QQuickText::componentComplete();
    connect(parentItem(), SIGNAL(effectiveHorizontalAlignmentChanged()), this, SLOT(updateAlignment()));
    updateAlignment();
}

void QQuickPlaceholderText::updateAlignment()
{
    if (QQuickTextInput *input = qobject_cast<QQuickTextInput *>(parentItem())) {
        if (QQuickTextInputPrivate::get(input)->hAlignImplicit)
            resetHAlign();
        else
            setHAlign(static_cast<HAlignment>(input->hAlign()));
    } else if (QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(parentItem())) {
        if (QQuickTextEditPrivate::get(edit)->hAlignImplicit)
            resetHAlign();
        else
            setHAlign(static_cast<HAlignment>(edit->hAlign()));
    } else {
        resetHAlign();
    }
}

QT_END_NAMESPACE

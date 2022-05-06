/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QTQUICKFOREIGN_P_H
#define QTQUICKFOREIGN_P_H

#include <qtquickglobal_p.h>

#if QT_CONFIG(im)
#include <QtGui/qinputmethod.h>
#endif
#if QT_CONFIG(validator)
#include <QtGui/qvalidator.h>
#endif
#if QT_CONFIG(shortcut)
#include <QtGui/qkeysequence.h>
#endif

#include <QtQml/qqml.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

#if QT_CONFIG(validator)
struct QValidatorForeign
{
    Q_GADGET
    QML_FOREIGN(QValidator)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
};

#if QT_CONFIG(regularexpression)
struct QRegularExpressionValidatorForeign
{
    Q_GADGET
    QML_FOREIGN(QRegularExpressionValidator)
    QML_NAMED_ELEMENT(RegularExpressionValidator)
    QML_ADDED_IN_VERSION(2, 14)
};
#endif // QT_CONFIG(regularexpression)

#endif // QT_CONFIG(validator)

#if QT_CONFIG(im)
struct QInputMethodForeign
{
    Q_GADGET
    QML_FOREIGN(QInputMethod)
    QML_NAMED_ELEMENT(InputMethod)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("InputMethod is an abstract class.")
};
#endif // QT_CONFIG(im)

#if QT_CONFIG(shortcut)
namespace QKeySequenceForeign
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QKeySequence)
    QML_NAMED_ELEMENT(StandardKey)
    QML_ADDED_IN_VERSION(2, 2)
};
#endif // QT_CONFIG(shortcut)

QT_END_NAMESPACE

#endif // QTQUICKFOREIGN_P_H

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

#ifndef QQMLJSDIAGNOSTICMESSAGE_P_H
#define QQMLJSDIAGNOSTICMESSAGE_P_H

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

#include <QtCore/qlogging.h>
#include <QtCore/qstring.h>

// Include the API version here, to avoid complications when querying it for the
// QQmlSourceLocation -> line/column change.
#include <private/qqmlapiversion_p.h>

#include "qqmljssourcelocation_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
struct DiagnosticMessage
{
    QString message;
    QtMsgType type = QtCriticalMsg;
    SourceLocation loc;

    bool isError() const
    {
        return type == QtCriticalMsg;
    }

    bool isWarning() const
    {
        return type == QtWarningMsg;
    }

    bool isValid() const
    {
        return !message.isEmpty();
    }
};
} // namespace QQmlJS

Q_DECLARE_TYPEINFO(QQmlJS::DiagnosticMessage, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLJSDIAGNOSTICMESSAGE_P_H

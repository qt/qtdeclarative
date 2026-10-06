// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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

#include "qqmljssourcelocation_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
enum class DiagnosticCategory : quint8 {
    // warnings -> QtWarningMsg
    AliasCycle,
    DuplicateInlineComponent,
    DuplicatePropertyBinding,
    DuplicatedName,
    Import,

    // errors -> QtCriticalMsg
    Syntax,
    SyntaxDuplicateIds,
};

struct MsgTypeOrDiagnosticCategory
{
public:
    MsgTypeOrDiagnosticCategory() : m_isQtMsgType(1), m_data(QtCriticalMsg) { }
    MsgTypeOrDiagnosticCategory(QtMsgType qtMsgType) : m_isQtMsgType(1), m_data(qtMsgType) { }
    MsgTypeOrDiagnosticCategory(DiagnosticCategory diagnosticCategory)
        : m_isQtMsgType(0), m_data(static_cast<quint8>(diagnosticCategory))
    {
    }
    friend bool comparesEqual(MsgTypeOrDiagnosticCategory a, MsgTypeOrDiagnosticCategory b) noexcept
    {
        return a.m_isQtMsgType == b.m_isQtMsgType && a.m_data == b.m_data;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(MsgTypeOrDiagnosticCategory);

    QtMsgType asMsgType() const
    {
        if (m_isQtMsgType)
            return static_cast<QtMsgType>(m_data);
        return static_cast<DiagnosticCategory>(m_data) < DiagnosticCategory::Syntax ? QtWarningMsg
                                                                                    : QtCriticalMsg;
    }

    quint8 m_isQtMsgType : 1;
    quint8 m_data : 7;
};

struct DiagnosticMessage
{
    QString message;
    MsgTypeOrDiagnosticCategory type = QtCriticalMsg;
    SourceLocation loc;

    QtMsgType msgType() const { return type.asMsgType(); }

    bool isError() const { return msgType() == QtCriticalMsg; }

    bool isWarning() const { return msgType() == QtWarningMsg; }
};
} // namespace QQmlJS

Q_DECLARE_TYPEINFO(QQmlJS::DiagnosticMessage, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLJSDIAGNOSTICMESSAGE_P_H

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlcompletioncontextstrings_p.h"

CompletionContextStrings::CompletionContextStrings(QString code, qsizetype pos)
    : m_code(code), m_pos(pos)
{
    // computes the context just before pos in code.
    // After this code all the values of all the attributes should be correct (see above)
    // handle also letter or numbers represented a surrogate pairs?
    m_filterStart = m_pos;
    while (m_filterStart != 0) {
        QChar c = code.at(m_filterStart - 1);
        if (!c.isLetterOrNumber() && c != u'_')
            break;
        else
            --m_filterStart;
    }
    // handle spaces?
    m_baseStart = m_filterStart;
    while (m_baseStart != 0) {
        QChar c = code.at(m_baseStart - 1);
        if (c != u'.' || m_baseStart == 1)
            break;
        c = code.at(m_baseStart - 2);
        if (!c.isLetterOrNumber() && c != u'_')
            break;
        qsizetype baseEnd = --m_baseStart;
        while (m_baseStart != 0) {
            QChar c = code.at(m_baseStart - 1);
            if (!c.isLetterOrNumber() && c != u'_')
                break;
            else
                --m_baseStart;
        }
        if (m_baseStart == baseEnd)
            break;
    }
    m_atLineStart = true;
    m_lineStart = m_baseStart;
    while (m_lineStart != 0) {
        QChar c = code.at(m_lineStart - 1);
        if (c == u'\n' || c == u'\r')
            break;
        if (!c.isSpace())
            m_atLineStart = false;
        --m_lineStart;
    }
}

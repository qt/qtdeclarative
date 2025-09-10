// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmnemoniclabel_p.h"

#include <QtQuick/private/qquicktext_p_p.h>

QT_BEGIN_NAMESPACE

QQuickMnemonicLabel::QQuickMnemonicLabel(QQuickItem *parent)
    : QQuickText(parent)
{
}

QString QQuickMnemonicLabel::text() const
{
    return m_fullText;
}

void QQuickMnemonicLabel::setText(const QString &text)
{
    if (m_fullText == text)
        return;

    m_fullText = text;
    updateMnemonic();
}

bool QQuickMnemonicLabel::isMnemonicVisible() const
{
    return m_mnemonicVisible;
}

void QQuickMnemonicLabel::setMnemonicVisible(bool visible)
{
    if (m_mnemonicVisible == visible)
        return;

    m_mnemonicVisible = visible;
    updateMnemonic();

    if (isComponentComplete())
        forceLayout();
}

static QTextLayout::FormatRange underlineRange(int start, int length = 1)
{
    QTextLayout::FormatRange range;
    range.start = start;
    range.length = length;
    range.format.setFontUnderline(true);
    return range;
}

// based on QPlatformTheme::removeMnemonics()
void QQuickMnemonicLabel::updateMnemonic()
{
    QString text(m_fullText.size(), QChar::Null);
    int idx = 0;
    int pos = 0;
    int len = m_fullText.size();
    QList<QTextLayout::FormatRange> formats;
    while (len) {
        if (m_fullText.at(pos) == QLatin1Char('&') && (len == 1 || m_fullText.at(pos + 1) != QLatin1Char('&'))) {
            if (m_mnemonicVisible && (pos == 0 || m_fullText.at(pos - 1) != QLatin1Char('&')))
                formats += underlineRange(pos);
            ++pos;
            --len;
            if (len == 0)
                break;
        } else if (m_fullText.at(pos) == QLatin1Char('(') && len >= 4 &&
                   m_fullText.at(pos + 1) == QLatin1Char('&') &&
                   m_fullText.at(pos + 2) != QLatin1Char('&') &&
                   m_fullText.at(pos + 3) == QLatin1Char(')')) {
            // a mnemonic with format "\s*(&X)"
            if (m_mnemonicVisible) {
                formats += underlineRange(pos + 1);
            } else {
                int n = 0;
                while (idx > n && text.at(idx - n - 1).isSpace())
                    ++n;
                idx -= n;
                pos += 4;
                len -= 4;
                continue;
            }
        }
        text[idx] = m_fullText.at(pos);
        ++pos;
        ++idx;
        --len;
    }
    text.truncate(idx);

    QQuickTextPrivate::get(this)->layout.setFormats(formats);
    QQuickText::setText(text);
}

QT_END_NAMESPACE

#include "moc_qquickmnemoniclabel_p.cpp"

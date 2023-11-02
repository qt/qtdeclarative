// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickmnemoniclabel_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQuick/private/qquicktext_p_p.h>

QT_BEGIN_NAMESPACE

QQuickMnemonicLabel::QQuickMnemonicLabel(QQuickItem *parent)
    : QQuickText(parent)
{
    m_mnemonicEnabled = QGuiApplicationPrivate::platformTheme()->themeHint(
        QPlatformTheme::MnemonicsEnabled).toBool();
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

/*!
    \internal

    This property determines whether \c "&" in \l text is treated as a
    mnemonic marker (\c true) or displayed literally (\c false).

    If \c true, QQuickMnemonicLabel parses \c "&" as a mnemonic marker and
    removes it from the displayed text, underlining the character that
    follows it (see \c updateMnemonic()). The character is not underlined when
    \c QPlatformTheme::UnderlineShortcut is \c false for the current platform,
    but the \c "&" is still removed.

    If \c false, \c "&" has no special meaning and is displayed literally,
    which allows it to be used in text such as \c "Cats & Dogs".

    For example, given \l text of \c "M&nemonic":
    \list
    \li \c true: the displayed text is \c "Mnemonic", with the \c "n" underlined
        (depending on \c QPlatformTheme::UnderlineShortcut).
    \li \c false: the displayed text is \c "M&nemonic" (unchanged).
    \endlist

    The default value is \c true if the platform supports mnemonics
    (\c QPlatformTheme::MnemonicsEnabled), otherwise \c false.
*/
bool QQuickMnemonicLabel::isMnemonicEnabled() const
{
    return m_mnemonicEnabled;
}

void QQuickMnemonicLabel::setMnemonicEnabled(bool enabled)
{
    if (m_mnemonicEnabled == enabled)
        return;

    m_mnemonicEnabled = enabled;
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
    if (!m_mnemonicEnabled) {
        // "&" has no special meaning; display the text as-is.
        QQuickTextPrivate::get(this)->layout.setFormats({});
        QQuickText::setText(m_fullText);
        return;
    }

    const bool showUnderline = QGuiApplicationPrivate::platformTheme()->themeHint(
        QPlatformTheme::UnderlineShortcut).toBool();

    QString text(m_fullText.size(), QChar::Null);
    int idx = 0;
    int pos = 0;
    int len = m_fullText.size();
    QList<QTextLayout::FormatRange> formats;
    while (len) {
        if (m_fullText.at(pos) == QLatin1Char('&') && (len == 1 || m_fullText.at(pos + 1) != QLatin1Char('&'))) {
            // A plain mnemonic marker, e.g. "M&nemonic": drop the "&" and
            // underline the character that follows it.
            if (showUnderline && (pos == 0 || m_fullText.at(pos - 1) != QLatin1Char('&')))
                formats += underlineRange(pos);
            ++pos;
            --len;
            if (len == 0)
                break;
        } else if (m_fullText.at(pos) == QLatin1Char('(') && len >= 4 &&
                   m_fullText.at(pos + 1) == QLatin1Char('&') &&
                   m_fullText.at(pos + 2) != QLatin1Char('&') &&
                   m_fullText.at(pos + 3) == QLatin1Char(')')) {
            // A mnemonic with format "\s*(&X)", used when the label itself has
            // no natural character to underline (e.g. non-Latin scripts). Keep
            // "X" in the text, and underline it if the platform draws
            // underlines for shortcuts.
            if (showUnderline)
                formats += underlineRange(pos + 1);
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

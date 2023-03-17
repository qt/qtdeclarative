// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ThirdPartyDisplay.h"
#include <QDebug>

const QString &ThirdPartyDisplay::content() const
{
    return m_content;
}

void ThirdPartyDisplay::setContent(const QString &content)
{
    if (m_content != content) {
        m_content = content;
        emit contentChanged();
    }
    qInfo() << QStringLiteral("[Fancy ThirdPartyDisplay] ") + content;
}

QColor ThirdPartyDisplay::foregroundColor() const
{
    return m_foregroundColor;
}

void ThirdPartyDisplay::setForegroundColor(QColor color)
{
    if (m_foregroundColor != color) {
        m_foregroundColor = color;
        emit colorsChanged();
    }
}

QColor ThirdPartyDisplay::backgroundColor() const
{
    return m_backgroundColor;
}

void ThirdPartyDisplay::setBackgroundColor(QColor color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        emit colorsChanged();
    }
}

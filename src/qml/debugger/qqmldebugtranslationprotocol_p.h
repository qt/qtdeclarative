
/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QQMLDEBUGTRANSLATIONPROTOCOL_P_H
#define QQMLDEBUGTRANSLATIONPROTOCOL_P_H
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
#include <QtCore/qdatastream.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qurl.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qmetaobject.h>
#include <tuple>

QT_BEGIN_NAMESPACE

namespace QQmlDebugTranslation {

enum class Request {
    ChangeLanguage = 1,
    StateList,
    ChangeState,
    TranslationIssues,
    TranslatableTextOccurrences,
    WatchTextElides,
    DisableWatchTextElides,
    // following are obsolete, just provided for compilation compatibility
    MissingTranslations
};

enum class Reply {
    LanguageChanged = 101,
    StateList,
    StateChanged,
    TranslationIssues,
    TranslatableTextOccurrences,
    // following are obsolete, just provided for compilation compatibility
    MissingTranslations,
    TextElided
};

inline QByteArray createChangeLanguageRequest(QDataStream &packet, const QUrl &url,
                                              const QString &locale)
{
    packet << Request::ChangeLanguage << url << locale;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

inline QByteArray createChangeStateRequest(QDataStream &packet, const QString &state)
{
    packet << Request::ChangeState << state;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

inline QByteArray createMissingTranslationsRequest(QDataStream &packet)
{
    packet << Request::MissingTranslations;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

inline QByteArray createTranslationIssuesRequest(QDataStream &packet)
{
    packet << Request::TranslationIssues;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

inline QByteArray createTranslatableTextOccurrencesRequest(QDataStream &packet)
{
    packet << Request::TranslatableTextOccurrences;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

inline QByteArray createStateListRequest(QDataStream &packet)
{
    packet << Request::StateList;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

inline QByteArray createWatchTextElidesRequest(QDataStream &packet)
{
    packet << Request::WatchTextElides;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

inline QByteArray createDisableWatchTextElidesRequest(QDataStream &packet)
{
    packet << Request::DisableWatchTextElides;
    return qobject_cast<QBuffer *>(packet.device())->data();
}

class CodeMarker
{
public:
    friend QDataStream &operator>>(QDataStream &stream, CodeMarker &codeMarker)
    {
        return stream >> codeMarker.url
                      >> codeMarker.line
                      >> codeMarker.column;
    }

    friend QDataStream &operator<<(QDataStream &stream, const CodeMarker &codeMarker)
    {
        return stream << codeMarker.url
                      << codeMarker.line
                      << codeMarker.column;
    }

    friend bool operator<(const CodeMarker &first, const CodeMarker &second)
    {
        return std::tie(first.url, first.line, first.column)
                < std::tie(second.url, second.line, second.column);
    }

    friend bool operator==(const CodeMarker &first, const CodeMarker &second)
    {
        return first.line == second.line
                && first.column == second.column
                && first.url == second.url;
    }

    QUrl url;
    int line = -1;
    int column = -1;
};
class TranslationIssue
{
public:
    enum class Type{
        Missing,
        Elided
    };

    friend QDataStream &operator>>(QDataStream &stream, TranslationIssue &issue)
    {
        return stream >> issue.codeMarker
                      >> issue.language
                      >> issue.type;
    }

    friend QDataStream &operator<<(QDataStream &stream, const TranslationIssue &issue)
    {
        return stream << issue.codeMarker
                      << issue.language
                      << issue.type;
    }

    friend bool operator==(const TranslationIssue &first, const TranslationIssue &second)
    {
        return first.type == second.type
                && first.language == second.language
                && first.codeMarker == second.codeMarker;
    }

    QString toDebugString() const
    {
        QString debugString(QLatin1String(
                "TranslationIssue(type=%1, line=%2, column=%3, url=%4, language=%5)"));
        return debugString.arg(type == TranslationIssue::Type::Missing ? QLatin1String("Missing")
                                                                       : QLatin1String("Elided"),
                               QString::number(codeMarker.line), QString::number(codeMarker.column),
                               codeMarker.url.toString(), language);
    }

    QString language;
    Type type = Type::Missing;
    CodeMarker codeMarker;
};
class QmlElement
{
public:
    QmlElement() = default;

    friend QDataStream &operator>>(QDataStream &stream, QmlElement &qmlElement)
    {
        return stream >> qmlElement.codeMarker >> qmlElement.elementId >> qmlElement.elementType
                >> qmlElement.propertyName >> qmlElement.translationId >> qmlElement.translatedText
                >> qmlElement.fontFamily >> qmlElement.fontPointSize >> qmlElement.fontPixelSize
                >> qmlElement.fontStyleName >> qmlElement.horizontalAlignment
                >> qmlElement.verticalAlignment >> qmlElement.stateName;
    }

    friend QDataStream &operator<<(QDataStream &stream, const QmlElement &qmlElement)
    {
        return stream << qmlElement.codeMarker << qmlElement.elementId << qmlElement.elementType
                      << qmlElement.propertyName << qmlElement.translationId
                      << qmlElement.translatedText << qmlElement.fontFamily
                      << qmlElement.fontPointSize << qmlElement.fontPixelSize
                      << qmlElement.fontStyleName << qmlElement.horizontalAlignment
                      << qmlElement.verticalAlignment << qmlElement.stateName;
    }

    CodeMarker codeMarker;
    QString propertyName;
    QString translationId;
    QString translatedText;
    QString fontFamily;
    QString fontStyleName;
    QString elementId;
    QString elementType;
    qreal fontPointSize = 0.0;
    QString stateName;
    int fontPixelSize = 0;
    int horizontalAlignment = 0;
    int verticalAlignment = 0;
};

class QmlState
{
public:
    QmlState() = default;

    friend QDataStream &operator>>(QDataStream &stream, QmlState &qmlState)
    {
        return stream >> qmlState.name;
    }

    friend QDataStream &operator<<(QDataStream &stream, const QmlState &qmlState)
    {
        return stream << qmlState.name;
    }

    QString name;
};
}

QT_END_NAMESPACE

#endif // QQMLDEBUGTRANSLATIONPROTOCOL_P_H

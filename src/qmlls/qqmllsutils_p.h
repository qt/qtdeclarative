// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QLANGUAGESERVERUTILS_P_H
#define QLANGUAGESERVERUTILS_P_H

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

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

struct QQmlLSUtilsItemLocation
{
    int depth = -1;
    QQmlJS::Dom::DomItem domItem;
    QQmlJS::Dom::FileLocations::Tree fileLocation;
};

struct QQmlLSUtilsTextPosition
{
    int line;
    int character;
};

class QQmlLSUtils
{
public:
    static qsizetype textOffsetFrom(const QString &code, int row, int character);
    static QQmlLSUtilsTextPosition textRowAndColumnFrom(const QString &code, qsizetype offset);
    enum IgnoreWhitespace { Off, OnSameLine };
    static QList<QQmlLSUtilsItemLocation> itemsFromTextLocation(QQmlJS::Dom::DomItem file, int line,
                                                                int character,
                                                                IgnoreWhitespace = Off);
    static QQmlJS::Dom::FileLocations::Tree textLocationFromItem(QQmlJS::Dom::DomItem qmlObject);
    static QByteArray lspUriToQmlUrl(const QByteArray &uri);
    static QByteArray qmlUrlToLspUri(const QByteArray &url);
};

#endif // QLANGUAGESERVERUTILS_P_H

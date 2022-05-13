// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef LSPCUSTOMTYPES_H
#define LSPCUSTOMTYPES_H
#include <QtLanguageServer/private/qlanguageserverspec_p.h>

QT_BEGIN_NAMESPACE

namespace QLspSpecification {

class UriToBuildDirs
{
public:
    QByteArray baseUri = {};
    QList<QByteArray> buildDirs = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "baseUri", baseUri);
        field(w, "buildDirs", buildDirs);
    }
};

namespace Notifications {
constexpr auto AddBuildDirsMethod = "$/addBuildDirs";

class AddBuildDirsParams
{
public:
    QList<UriToBuildDirs> buildDirsToSet = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "buildDirsToSet", buildDirsToSet);
    }
};
} // namespace Notifications
} // namespace QLspSpecification

QT_END_NAMESPACE

#endif // LSPCUSTOMTYPES_H

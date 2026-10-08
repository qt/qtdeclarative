// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant reason:default

#ifndef QMLLSQUICKPLUGIN_H
#define QMLLSQUICKPLUGIN_H

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

#include <QtCore/qplugin.h>

#include <QtQmlLS/private/qqmllsplugin_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>

QT_BEGIN_NAMESPACE

class QQmlLSQuickCompletionPlugin : public QQmlLSCompletionPlugin
{
public:
    void suggestSnippetsForLeftHandSideOfBinding(const QQmlJS::Dom::DomItem &items,
                                                 BackInsertIterator result) const override;

    void suggestSnippetsForRightHandSideOfBinding(const QQmlJS::Dom::DomItem &items,
                                                  BackInsertIterator result) const override;
};


class QQmlLSQuickPlugin : public QObject, QQmlLSPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QmlLSPluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(QQmlLSPlugin)
public:
    std::unique_ptr<QQmlLSCompletionPlugin> createCompletionPlugin() const override;
};

QT_END_NAMESPACE

#endif // QMLLSQUICKPLUGIN_H

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLANGUAGESERVER_P_H
#define QQMLLANGUAGESERVER_P_H

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

#include <private/qqmldocumentsymbolsupport_p.h>
#include <private/qlanguageserver_p.h>
#include <private/qqmlcodemodelmanager_p.h>
#include <private/qqmlfindusagessupport_p.h>
#include <private/qtextsynchronization_p.h>
#include <private/qqmllintsuggestions_p.h>
#include <private/qworkspace_p.h>
#include <private/qqmlcompletionsupport_p.h>
#include <private/qqmlgototypedefinitionsupport_p.h>
#include <private/qqmlformatting_p.h>
#include <private/qqmlrangeformatting_p.h>
#include <private/qqmlgotodefinitionsupport_p.h>
#include <private/qqmlrenamesymbolsupport_p.h>
#include <private/qqmlhover_p.h>
#include <private/qqmlhighlightsupport_p.h>
#include <private/qqmlprogresssupport_p.h>
#include <private/qqmllscodeaction_p.h>

QT_BEGIN_NAMESPACE

class QQmlToolingSettings;

namespace QmlLsp {

class QQmlLanguageServer : public QLanguageServer
{
    Q_OBJECT
public:
    QQmlLanguageServer(std::function<void(const QByteArray &)> sendData,
                       QQmlToolingSharedSettings *settings = nullptr);
    ~QQmlLanguageServer();

    int returnValue() const;

    QQmlCodeModelManager *codeModelManager();

public Q_SLOTS:
    void exit();
    void errorExit();

private:
    QQmlCodeModelManager m_codeModelManager;

    TextSynchronization m_textSynchronization;
    WorkspaceHandlers m_workspace;

    // note: the order in which server modules are initialized also defines the order in which
    // they are run! Most of them connect to m_codeModelManager::updatedSnapshot in
    // their constructors, see https://doc.qt.io/qt-6/signalsandslots.html#signals.
    // ==== modules that are directly triggered by the user ====
    QmlCompletionSupport m_completionSupport;
    QmlGoToTypeDefinitionSupport m_navigationSupport;
    QmlGoToDefinitionSupport m_definitionSupport;
    QQmlFindUsagesSupport m_referencesSupport;
    QQmlDocumentFormatting m_documentFormatting;
    QQmlRenameSymbolSupport m_renameSupport;
    QQmlRangeFormatting m_rangeFormatting;
    QQmlHover m_hover;
    QQmlCodeActionSupport m_codeActionSupport;

    // ==== Highlighting ====
    QQmlHighlightSupport m_highlightSupport;

    // ==== modules that are not triggered by the user ====
    QQmlDocumentSymbolSupport m_documentSymbolSupport;
    QQmlProgressSupport m_progressSupport;

    // ==== Linting should happen at the end as it potentially can take a longer time ====
    QmlLintSuggestions m_lint;
    int m_returnValue = 1;
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLLANGUAGESERVER_P_H

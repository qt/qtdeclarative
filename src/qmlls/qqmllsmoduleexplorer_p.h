// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLSMODULEEXPLORER_P_H
#define QQMLLSMODULEEXPLORER_P_H

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

#include <QtCore/qhash.h>
#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>
#include <QtCore/qwaitcondition.h>

#include <deque>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QQmlLSModuleExplorer : public QObject
{
    Q_OBJECT
public:
    QQmlLSModuleExplorer();
    ~QQmlLSModuleExplorer() override;

    void setEnabled(bool enabled);
    void explore(const QStringList &importPaths);
    void cancel();
    QStringList modulesIn(const QStringList &importPaths) const;
    bool isExploring() const;

Q_SIGNALS:
    void explorationStarted();
    void explorationFinished();

private:
    void run();
    std::optional<QString> takeNextImportPath();
    void addModule(const QString &importPath, const QString &module);
    bool finishExploring();

    std::unique_ptr<QThread> m_thread;
    mutable QMutex m_mutex;
    QWaitCondition m_hasWork;
    std::deque<QString> m_pendingImportPaths;
    QHash<QString, QStringList> m_modulesPerImportPath;
    bool m_enabled = true;
    bool m_exploring = false;
    bool m_stopRequested = false;
    bool m_cancelRequested = false;
};

QT_END_NAMESPACE

#endif // QQMLLSMODULEEXPLORER_P_H

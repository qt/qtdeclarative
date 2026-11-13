// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLLS_PROGRESS_P_H
#define TST_QMLLS_PROGRESS_P_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class tst_qmlls_progress: public QObject
{
    Q_OBJECT
public:
    tst_qmlls_progress() { }

private slots:
    void backgroundBuild_data();
    void backgroundBuild();

    void cancelBackgroundBuild();
    void cancelBackgroundBuildWithInvalidToken();
    void orderOfProgressNotifications();

    void moduleExploration_data();
    void moduleExploration();
    void cancelModuleExploration();
    void moduleExplorationStartedBeforeClientInitialized();
    void moduleExplorationAndBackgroundBuildHaveDistinctTokens();
};

QT_END_NAMESPACE
#endif // TST_QMLLS_PROGRESS_P_H

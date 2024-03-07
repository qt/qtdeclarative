// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LIFECYCLEWATCHER_H
#define LIFECYCLEWATCHER_H

#include <QtQml/qqmlparserstatus.h>
#include <private/qqmlfinalizer_p.h>
#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class LifeCycleWatcher : public QObject, public QQmlParserStatus, public QQmlFinalizerHook
{
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_INTERFACES(QQmlFinalizerHook)
public:
    void classBegin() override {states.push_back(1); }
    void componentComplete() override {states.push_back(2);};
    void componentFinalized() override { states.push_back(3); }
    QList<int> states;
};
#endif

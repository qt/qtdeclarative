/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QUICKTESTUTIL_P_H
#define QUICKTESTUTIL_P_H

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

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

class QuickTestUtil : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool printAvailableFunctions READ printAvailableFunctions NOTIFY printAvailableFunctionsChanged)
    Q_PROPERTY(int dragThreshold READ dragThreshold NOTIFY dragThresholdChanged)
    QML_NAMED_ELEMENT(TestUtil)
public:
    QuickTestUtil(QObject *parent = nullptr) :QObject(parent) {}
    ~QuickTestUtil() override {}

    bool printAvailableFunctions() const;
    int dragThreshold() const;

Q_SIGNALS:
    void printAvailableFunctionsChanged();
    void dragThresholdChanged();

public Q_SLOTS:

    QJSValue typeName(const QVariant& v) const;
    bool compare(const QVariant& act, const QVariant& exp) const;

    QJSValue callerFile(int frameIndex = 0) const;
    int callerLine(int frameIndex = 0) const;
};

QT_END_NAMESPACE

#endif // QUICKTESTUTIL_P_H

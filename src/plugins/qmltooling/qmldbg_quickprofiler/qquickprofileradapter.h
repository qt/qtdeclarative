// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKPROFILERADAPTER_H
#define QQUICKPROFILERADAPTER_H

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

#include <QtQml/private/qqmlabstractprofileradapter_p.h>
#include <QtQuick/private/qquickprofiler_p.h>

QT_BEGIN_NAMESPACE

class QQuickProfilerAdapter : public QQmlAbstractProfilerAdapter {
    Q_OBJECT
public:
    QQuickProfilerAdapter(QObject *parent = nullptr);
    ~QQuickProfilerAdapter();
    qint64 sendMessages(qint64 until, QList<QByteArray> &messages) override;
    void receiveData(const QVector<QQuickProfilerData> &new_data);

private:
    int next;
    QVector<QQuickProfilerData> m_data;
};

QT_END_NAMESPACE

#endif // QQUICKPROFILERADAPTER_H

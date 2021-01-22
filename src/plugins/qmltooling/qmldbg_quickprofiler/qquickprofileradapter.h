/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
    QQuickProfilerAdapter(QObject *parent = 0);
    ~QQuickProfilerAdapter();
    qint64 sendMessages(qint64 until, QList<QByteArray> &messages) override;
    void receiveData(const QVector<QQuickProfilerData> &new_data);

private:
    int next;
    QVector<QQuickProfilerData> m_data;
};

QT_END_NAMESPACE

#endif // QQUICKPROFILERADAPTER_H

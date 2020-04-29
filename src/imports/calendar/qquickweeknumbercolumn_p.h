/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Calendar module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKWEEKNUMBERCOLUMN_P_H
#define QQUICKWEEKNUMBERCOLUMN_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickWeekNumberColumnPrivate;

class QQuickWeekNumberColumn : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY yearChanged FINAL)
    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)

public:
    explicit QQuickWeekNumberColumn(QQuickItem *parent = nullptr);

    int month() const;
    void setMonth(int month);

    int year() const;
    void setYear(int year);

    QVariant source() const;
    void setSource(const QVariant &source);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

Q_SIGNALS:
    void monthChanged();
    void yearChanged();
    void sourceChanged();
    void delegateChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override;
    void paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding) override;

private:
    Q_DISABLE_COPY(QQuickWeekNumberColumn)
    Q_DECLARE_PRIVATE(QQuickWeekNumberColumn)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickWeekNumberColumn)

#endif // QQUICKWEEKNUMBERCOLUMN_P_H

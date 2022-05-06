/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKCOLORIMAGE_P_H
#define QQUICKCOLORIMAGE_P_H

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

#include <QtGui/qcolor.h>
#include <QtQuick/private/qquickimage_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2_PRIVATE_EXPORT QQuickColorImage : public QQuickImage
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor RESET resetColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QColor defaultColor READ defaultColor WRITE setDefaultColor RESET resetDefaultColor NOTIFY defaultColorChanged FINAL)
    QML_NAMED_ELEMENT(ColorImage)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickColorImage(QQuickItem *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);
    void resetColor();

    QColor defaultColor() const;
    void setDefaultColor(const QColor &color);
    void resetDefaultColor();

Q_SIGNALS:
    void colorChanged();
    void defaultColorChanged();

protected:
    void pixmapChange() override;

private:
    QColor m_color = Qt::transparent;
    QColor m_defaultColor = Qt::transparent;
};

QT_END_NAMESPACE

#endif // QQUICKCOLORIMAGE_P_H

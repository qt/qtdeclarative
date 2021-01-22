/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#ifndef QSGCONTEXTPLUGIN_H
#define QSGCONTEXTPLUGIN_H

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

#include <private/qtquickglobal_p.h>
#include <QtQuick/qquickimageprovider.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE

class QSGContext;

class QSGRenderLoop;

struct Q_QUICK_PRIVATE_EXPORT QSGContextFactoryInterface : public QFactoryInterface
{
    enum Flag {
        SupportsShaderEffectNode = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    virtual QSGContext *create(const QString &key) const = 0;
    virtual Flags flags(const QString &key) const = 0;

    virtual QQuickTextureFactory *createTextureFactoryFromImage(const QImage &image) = 0;
    virtual QSGRenderLoop *createWindowManager() = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGContextFactoryInterface::Flags)

#define QSGContextFactoryInterface_iid \
        "org.qt-project.Qt.QSGContextFactoryInterface"
Q_DECLARE_INTERFACE(QSGContextFactoryInterface, QSGContextFactoryInterface_iid)

class Q_QUICK_PRIVATE_EXPORT QSGContextPlugin : public QObject, public QSGContextFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QSGContextFactoryInterface:QFactoryInterface)
public:
    explicit QSGContextPlugin(QObject *parent = nullptr);
    virtual ~QSGContextPlugin();

    QStringList keys() const override = 0;

    QQuickTextureFactory *createTextureFactoryFromImage(const QImage &) override { return nullptr; }
    QSGRenderLoop *createWindowManager() override { return nullptr; }
};

QT_END_NAMESPACE

#endif // QSGCONTEXTPLUGIN_H

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QFile>
#include <QtQuick/private/qquickimage_p.h>

class DeviceLoadingImage : public QQuickImage
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DeviceLoadingImage)

public:
    DeviceLoadingImage(QQuickItem *parent = nullptr) : QQuickImage(parent) { }

protected:
    void load() override;

protected slots:
    void onRequestFinished();

public:
    std::unique_ptr<QFile> device;
    bool connectSuccess = true;
    int requestsFinished = 0;
};

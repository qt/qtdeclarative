// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTYLEREADER_H
#define QSTYLEREADER_H

#include <QtCore>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>

#include "jsontools.h"

using namespace JsonTools;

class StyleGenerator {

public:
    StyleGenerator(const QJsonDocument &document, const QString &resourcePath, const QString &targetPath)
        : m_document(document)
        , m_resourcePath(resourcePath)
        , m_targetPath(targetPath)
    {
    }

    void setVerbose(bool verbose)
    {
        m_verbose = verbose;
    }

    void generateStyle()
    {
        debug("resource path: " + m_resourcePath);
        debug("target path: " + m_targetPath);

        generateButton();
        generateCheckBox();
        generateSwitch();
        generateQmlDir();
        generateConfig();
    }

private:

    void generateButton()
    {
        generateControl("Button");

        const QMap<QString, QString> stateMap = {
            {"idle", ""},
            {"blocked", "disabled"}
        };

        forEachStateIn("ButtonTemplate",
            [this, &stateMap](const QString &state, const QJsonObject &artboard){
            const QString imageState = stateMap.value(state, state);
            copyImageToStyleFolder({"background"}, "button-background", imageState, artboard);

            const QRectF bgGeo = getGeometry({"background"}, artboard);
            const QRectF contentGeo = getGeometry({"label"}, artboard);

            QJsonObject desc;
            desc.insert("width", bgGeo.width());
            desc.insert("height", bgGeo.height());
            desc.insert("leftPadding", contentGeo.x());
            desc.insert("topPadding", contentGeo.y());
            desc.insert("rightPadding", qMax(0., bgGeo.width() - contentGeo.x() - contentGeo.width()));
            desc.insert("bottomPadding", qMax(0., bgGeo.height() - contentGeo.y() - contentGeo.height()));
            // Todo: use the radius of the rectangle to decide the stretch
            desc.insert("horizontalStretch", QJsonArray{8, bgGeo.width() - 8});
            desc.insert("verticalStretch", QJsonArray{8, bgGeo.width() - 8});

            m_imageConfigRoot.insert(imageName("button-background", imageState), desc);
        });
    }

    void generateCheckBox()
    {
        generateControl("CheckBox");

        const QMap<QString, QString> stateMap = {
            {"idle", ""},
            {"blocked", "disabled"},
            {"checkedBlocked", "disabled-checked"},
            {"checkedPressed", "checked-pressed"},
            {"checkedHovered", "checked-hovered"},
            {"checkedTri", "tristate"},
            {"blockedTri", "tristate-disabled"},
            {"pressedTri", "tristate-pressed"},
            {"hoveredTri", "tristate-hovered"}
        };

        forEachStateIn("CheckboxTemplate",
            [this, &stateMap](const QString &state, const QJsonObject &artboard){
            const QString imageState = stateMap.value(state, state);
            copyImageToStyleFolder({"CheckboxBackground", "background"}, "checkbox-background", imageState, artboard);
            copyImageToStyleFolder({"CheckboxIndicator", "checkBackground"}, "checkbox-indicator-background", imageState, artboard);
            copyImageToStyleFolder({"CheckboxIndicator", "checkIcon"}, "checkbox-indicator-icon", imageState, artboard);
            copyImageToStyleFolder({"CheckboxIndicator", "triCheckIcon"}, "checkbox-indicator-partialicon", imageState, artboard);

            const QRectF bgGeo = getGeometry({"CheckboxBackground", "background"}, artboard);
            const QRectF contentGeo = getGeometry({"CheckboxBackground", "Label"}, artboard);
            const QRectF indicatorGeo = getGeometry({"CheckboxIndicator"}, artboard);
            const bool mirrored = indicatorGeo.x() > bgGeo.width() / 2;

            QJsonObject desc;
            desc.insert("width", bgGeo.width());
            desc.insert("height", bgGeo.height());
            desc.insert("mirrored", mirrored);
            // Todo: use the radius of the rectangle to decide the stretch
            desc.insert("horizontalStretch", QJsonArray{8, bgGeo.width() - 8});
            desc.insert("verticalStretch", QJsonArray{8, bgGeo.height() - 8});

            if (mirrored) {
                // When we detect that the indicator is on the right side of
                // the background, we create a control that is mirrored.
                desc.insert("leftPadding", contentGeo.x());
                desc.insert("rightPadding", qMax(0., bgGeo.width() - indicatorGeo.x() - indicatorGeo.width()));
                desc.insert("spacing", qMax(0., indicatorGeo.x() - contentGeo.x() - contentGeo.width()));
            } else {
                desc.insert("leftPadding", indicatorGeo.x());
                desc.insert("rightPadding",  qMax(0., bgGeo.width() - contentGeo.x() - contentGeo.width()));
                desc.insert("spacing", qMax(0., contentGeo.x() - indicatorGeo.x() - indicatorGeo.width()));
            }

            m_imageConfigRoot.insert(imageName("checkbox-background", imageState), desc);
        });
    }

    void generateSwitch()
    {
        generateControl("Switch");

        const QMap<QString, QString> stateMap = {
            {"idleON", "checked"},
            {"idleOFF", ""},
            {"pressedON", "checked-pressed"},
            {"pressedOFF", "pressed"},
            {"blocked", "disabled"}
        };

        forEachStateIn("SwitchTemplate",
            [this, &stateMap](const QString &state, const QJsonObject &artboard){
            const QString imageState = stateMap.value(state, state);
            copyImageToStyleFolder({"SwitchBackground", "background"}, "switch-background", imageState, artboard);
            copyImageToStyleFolder({"SwitchBackground", "iconLeftOff"}, "switch-background-lefticon", imageState, artboard);
            copyImageToStyleFolder({"SwitchBackground", "iconRightOff"}, "switch-background-righticon", imageState, artboard);
            copyImageToStyleFolder({"SwitchHandle", "handle"}, "switch-handle", imageState, artboard);
            copyImageToStyleFolder({"SwitchHandle", "iconLeftON"}, "switch-handle-lefticon", imageState, artboard);
            copyImageToStyleFolder({"SwitchHandle", "iconRightON"}, "switch-handle-righticon", imageState, artboard);
        });
    }

    void generateControl(const QString &name)
    {
        debugHeader(name);
        copyFileToStyleFolder(":/" + name + ".qml");
        m_qmlDirControls.append(name);
    }

    void generateConfig()
    {
        QJsonObject root;
        root.insert("version", "1.0");
        root.insert("images", m_imageConfigRoot);
        QJsonDocument configDocument(root);

        const QString path = m_targetPath + "/config.json";
        const QString styleName = QFileInfo(m_targetPath).fileName();
        debug("generating configuration: " + path);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            throw std::runtime_error("Could not open file for writing: " + path.toStdString());

        QTextStream out(&file);
        out << configDocument.toJson();
    }

    void generateQmlDir()
    {
        const QString path = m_targetPath + "/qmldir";
        const QString styleName = QFileInfo(m_targetPath).fileName();
        debug("generating qmldir: " + path);

        const QString version(" 1.0 ");
        QString qmldir;
        qmldir += "module " + styleName + "\n";
        for (const QString &control : m_qmlDirControls)
            qmldir += control + version + control + ".qml\n";

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            throw std::runtime_error("Could not open file for writing: " + path.toStdString());

        QTextStream out(&file);
        out << qmldir;
    }

    /**
     * Copies the given file into the style folder.
     * If destFileName is empty, the file name of the src will be used.
     */
    void copyFileToStyleFolder(const QString srcPath, const QString destFileName = "")
    {
        QFile srcFile = QFile(srcPath);
        if (!srcFile.exists())
            throw std::runtime_error("File doesn't exist: " + srcPath.toStdString());

        QString fileName = destFileName;
        if (fileName.isEmpty())
            fileName = QFileInfo(srcFile).fileName();

        const QString targetFilePath = m_targetPath + "/" + fileName;
        QFileInfo targetFilePathInfo(targetFilePath);
        const QDir targetDir = targetFilePathInfo.absoluteDir();
        if (!targetDir.exists())
        {
            if (!QDir().mkpath(targetDir.path()))
                throw std::runtime_error("Could not create target path: " + m_targetPath.toStdString());
        }

        debug("copying " + QFileInfo(srcPath).fileName() + " to " + targetFilePath);
        srcFile.copy(targetFilePath);
    }

    void copyImageToStyleFolder(
        const QJsonObject objectWithAsset,
        const QString &imageBaseName,
        const QString &imageState)
    {
        // Require the images to be png for now. While we could convert svg's to
        // png's on the fly, we should rather investigate how we can do this during build
        // time (with the work done to create png icons from svg from cmake).
        const QString resourceName = getImagePath(objectWithAsset);
        if (!resourceName.endsWith(".png"))
            throw std::runtime_error("The image needs to be png: " + resourceName.toStdString());

        const QString srcPath = m_resourcePath + '/' + resourceName;
        const QString targetName = "images/" + imageName(imageBaseName, imageState) + ".png";
        copyFileToStyleFolder(srcPath, targetName);
    }

    void copyImageToStyleFolder(
        const QStringList path,
        const QString &imageBaseName,
        const QString &imageState,
        const QJsonObject artboard)
    {
        try {
            const auto objectWithAsset = getChild(path, artboard, m_document);
            copyImageToStyleFolder(objectWithAsset, imageBaseName, imageState);
        } catch (NoImageFoundException &) {
            qWarning().nospace().noquote()
                << "Warning: no image found for: "
                << m_forEachStateIn_ErrorMsgHint << ", "
                << getChild_resolvedPath.join(", ");
        }
    }

    template <typename DelegateFunction>
    void forEachStateIn(const QString &artboardSetName, DelegateFunction fun)
    {
        const auto artboardSet = getArtboardSet(artboardSetName, m_document);
        const auto artboards = getArtboards(artboardSet);

        for (const QString &state : artboards.keys()) {
            try {
                debug("generating assets for state: " + state);
                m_forEachStateIn_ErrorMsgHint = artboardSetName + ", " + state;
                fun(state, artboards[state]);
            } catch (NoArtboardFoundException &e) {
                qWarning().nospace().noquote()
                    << "Warning: no artboard found for: '"
                    << m_forEachStateIn_ErrorMsgHint << ", "
                    << e.what() << "'";
            } catch (std::exception &e) {
                qWarning().nospace().noquote()
                    << "Warning: could not generate control: '"
                    << m_forEachStateIn_ErrorMsgHint << "'. "
                    << "Reason: " << e.what();
            }
        }
    }

    QRectF getGeometry(const QJsonObject object)
    {
        const auto x = getValue("x", object).toDouble();
        const auto y = getValue("y", object).toDouble();
        const auto width = getValue("width", object).toDouble();
        const auto height = getValue("height", object).toDouble();
        return QRectF(x, y, width, height);
    }

    QRectF getGeometry(const QStringList &path, const QJsonObject &parent)
    {
        return getGeometry(getChild(path, parent, m_document));
    }

    QString imageName(const QString &baseName, const QString &state) const
    {
        return baseName + (state.isEmpty() ? "" : "-" + state);
    }

    void debug(const QString &msg)
    {
        if (!m_verbose)
            return;
        qDebug().noquote() << msg;
    }

    void debugHeader(const QString &msg)
    {
        debug("");
        debug("*** " + msg + " ***");
    }

private:
    QJsonDocument m_document;
    QJsonObject m_imageConfigRoot;
    QStringList m_qmlDirControls;
    QString m_resourcePath;
    QString m_targetPath;
    QString m_forEachStateIn_ErrorMsgHint;
    bool m_verbose = false;
};

#endif // QSTYLEREADER_H

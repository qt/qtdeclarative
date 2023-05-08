// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTYLEREADER_H
#define QSTYLEREADER_H

#include <QtCore>
#include <QMargins>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDir>

#include "jsontools.h"

using namespace JsonTools;

class StyleGenerator {

public:
    StyleGenerator(const QString &fileId, const QString &token
        , const QString &targetPath, bool verbose, bool silent)
        : m_fileId(fileId)
        , m_token(token)
        , m_targetPath(targetPath)
        , m_verbose(verbose)
        , m_silent(silent)
    {
    }

    void generateStyle()
    {
        debug("Generate style: " + m_targetPath);

        downloadFigmaDocument();
        generateControls();
        downloadImages();
        debugHeader("Generating config files");
        generateQmlDir();
        generateConfiguration();
    }

private:

    void downloadFigmaDocument()
    {
        debug("downloading figma file");
        newProgress("downloading figma file");
        QJsonDocument figmaResponsDoc;
        const QUrl url("https://api.figma.com/v1/files/" + m_fileId);
        debug("requesting: " + url.toString());

        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("X-FIGMA-TOKEN"), m_token.toUtf8());
        debug("using token: " + m_token.toUtf8());

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);

        QNetworkReply *reply = manager->get(request);

        QObject::connect(reply, &QNetworkReply::finished, [this, &reply]{
            if (reply->error() == QNetworkReply::NoError)
                m_document = QJsonDocument::fromJson(reply->readAll());

            if (qgetenv("QSTYLEGENERATOR_SAVEDOC") == "true")
                saveForDebug(m_document.object(), "figmastyle.json");
        });

        QObject::connect(reply, &QNetworkReply::downloadProgress, [this]{
            progress();
        });

        while (reply->isRunning())
            qApp->processEvents();

        if (reply->error() != QNetworkReply::NoError)
            throw RestCallException(QStringLiteral("Could not download design file from Figma: ")
                + networkErrorString(reply));
    }

    QJsonDocument generateImageUrls()
    {
        newProgress("downloading image urls");
        QJsonDocument figmaResponsDoc;
        const QString ids = QStringList(m_imagesToDownload.keys()).join(",");
        const QUrl url("https://api.figma.com/v1/images/" + m_fileId + "?ids=" + ids);
        debug("request: " + url.toString());
        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("X-FIGMA-TOKEN"), m_token.toUtf8());

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);

        QNetworkReply *reply = manager->get(request);

        QObject::connect(reply, &QNetworkReply::finished, [reply, &figmaResponsDoc]{
            if (reply->error() == QNetworkReply::NoError)
                figmaResponsDoc = QJsonDocument::fromJson(reply->readAll());
        });

        QObject::connect(reply, &QNetworkReply::downloadProgress, [this]{
            progress();
        });

        while (reply->isRunning())
            qApp->processEvents();

        if (reply->error() != QNetworkReply::NoError)
            throw RestCallException(QStringLiteral("Could not download images from Figma: ")
                + networkErrorString(reply));

        return figmaResponsDoc;
    }

    void downloadImages(const QJsonDocument &figmaImagesResponsDoc)
    {
        debug("downloading requested images...");
        newProgress("downloading images");
        const auto imageUrls = getObject("images", figmaImagesResponsDoc.object());

        // Create image directory
        const QString imageDir = m_targetPath + "/images/";
        QFileInfo fileInfo(imageDir);
        const QDir targetDir = fileInfo.absoluteDir();
        if (!targetDir.exists()) {
            if (!QDir().mkpath(targetDir.path()))
                throw std::runtime_error("Could not create image directory: " + imageDir.toStdString());
        }

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);
        int requestCount = imageUrls.keys().count();

        for (const QString &key : imageUrls.keys()) {
            const QString imageUrl = imageUrls.value(key).toString();
            if (imageUrl.isEmpty()) {
                qWarning().noquote().nospace() << "No image URL generated for id: " << key << ", " << m_imagesToDownload.value(key);
                requestCount--;
                continue;
            }
            QNetworkReply *reply = manager->get(QNetworkRequest(imageUrl));

            QObject::connect(reply, &QNetworkReply::finished, [this, key, imageUrl, imageDir, reply, &requestCount] {
                requestCount--;
                if (reply->error() != QNetworkReply::NoError) {
                    qWarning().nospace().noquote() << "Failed to download "
                        << imageUrl << " (id: " << key << "). "
                        << "Error code:" << networkErrorString(reply);
                    return;
                }

                const QString name = m_imagesToDownload.value(key);
                const QString path = imageDir + name + ".png";

                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll(), "png");
                pixmap.save(path);
                debug("downloaded image: " + imageUrl + " (id: " + key + ")" + " to " + path);
                progress();
            });
        }

        while (requestCount > 0)
            qApp->processEvents();
    }

    void downloadImages()
    {
        debugHeader("Downloading images from Figma");
        if (m_imagesToDownload.isEmpty()) {
            debug("No images to download!");
            return;
        }

        try {
            downloadImages(generateImageUrls());
        } catch (std::exception &e) {
            qWarning().nospace().noquote() << "Could not generate images: " << e.what();
        }
    }

    void generateControls()
    {
        QFile file(":/config.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Could not open file for reading: " + file.fileName().toStdString());

        QJsonParseError parseError;
        QJsonDocument configDoc = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error(QString("Could not parse " + file.fileName()
                + ": " + parseError.errorString()).toStdString());

        const auto rootObject = configDoc.object();
        QJsonArray controlsArray;
        try {
            controlsArray = getArray("controls", rootObject);
        } catch (std::exception &e) {
            throw std::runtime_error("Could not parse " + file.fileName().toStdString() + ": " + e.what());
        }

        const QJsonArray exportArray = rootObject.value("default export").toArray();
        for (const QJsonValue &exportValue : exportArray)
            m_defaultExport.append(exportValue.toString());

        for (const auto controlValue : controlsArray) {
            const auto controlObj = controlValue.toObject();
            const QString name = getString("name", controlObj);
            try {
                generateControl(controlObj);
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Could not generate control: " << e.what();
            }
        }
    }

    void generateControl(const QJsonObject controlObj)
    {
        const QString controlName = getString("name", controlObj);
        debugHeader(controlName);

        QJsonObject outputControlConfig;

        // Get the description about the control from the input config document
        const auto configAtoms = getArray("atoms", controlObj);
        const auto componentSetName = getString("component set", controlObj);

        // Get the json object that describes the control in the Figma file
        const auto documentRoot = getObject("document", m_document.object());
        const auto componentSet = findChild({"type", "COMPONENT_SET", "name", componentSetName}, documentRoot);

        // Copy files (typically the QML control) into the style folder
        QStringList files = getStringList("copy", controlObj, false);
        for (const QString &file : files)
            copyFileToStyleFolder(file, false);

        // Add this control to the list of controls that goes into the qmldir file
        m_qmlDirControls.append(controlName);

        const auto configStatesArray = getArray("states", controlObj);
        for (const QJsonValue &configStateValue : configStatesArray) try {
            QJsonObject outputStateConfig;

            // Resolve all atoms for the given state
            m_currentAtomInfo = "control: " + controlName;
            const QJsonObject configStateObj = configStateValue.toObject();
            const QString controlState = getString("state", configStateObj);
            const QString figmaState = getString("figmaState", configStateObj);

            for (const QJsonValue &atomConfigValue : configAtoms) try {
                QJsonObject outputAtomConfig;
                m_currentAtomInfo = "control: " + controlName + "; state: " + controlState;
                const QJsonObject atomConfigObj = atomConfigValue.toObject();

                // Resolve the atom name. The atomConfigName cannot contain any
                // '-', since it will also be used as property name from QML.
                const QString atomName = getString("atom", atomConfigObj);
                m_currentAtomInfo += "; atom: " + atomName;
                QString atomConfigName = atomName;
                atomConfigName.replace('-', '_');

                // Resolve the path to the node in Figma
                const auto figmaPath = getString("figmaPath", atomConfigObj);
                m_currentAtomInfo += "; figma path: " + figmaPath;

                // Find the json object in the Figma document that describes the atom
                const auto figmaAtomObj = findAtomObject(figmaPath, figmaState, componentSet);

                // Add some convenience values into the config
                const auto figmaId = getString("id", figmaAtomObj);
                outputAtomConfig.insert("figmaId", figmaId);
                m_currentAtomInfo += "; id: " + figmaId;
                const QString atomCombinedName = controlName.toLower() + "-" + atomName
                    + (controlState == "normal" ? "" : "-" + controlState);
                outputAtomConfig.insert("name", atomCombinedName);

                // Export the atom
                QStringList customExportList = getStringList("export", atomConfigObj, false);
                const auto atomExportList = customExportList.isEmpty() ? m_defaultExport : customExportList;

                for (const QString &atomExport : atomExportList) try {
                    if (atomExport == "geometry")
                        exportGeometry(figmaAtomObj, outputAtomConfig);
                    else if (atomExport == "layout")
                        exportLayout(figmaAtomObj, outputAtomConfig);
                    else if (atomExport == "image")
                        exportImage(figmaAtomObj, outputAtomConfig);
                    else if (atomExport == "json")
                        exportJson(figmaAtomObj, outputAtomConfig);
                    else
                        throw std::runtime_error("Unknown option: '" + atomExport.toStdString() + "'");
                } catch (std::exception &e) {
                    qWarning().nospace().noquote() << "Warning, export atom: " << e.what() << "; " << m_currentAtomInfo;
                }

                // Add the exported atom configuration to the state configuration
                outputStateConfig.insert(atomConfigName, outputAtomConfig);
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, generate atom: " << e.what() << "; " << m_currentAtomInfo;
            }

            try {
                // Generate output configuration for the control as a whole. This involves
                // reading the output configuration from the already exported atoms.
                m_currentAtomInfo = "control: " + controlName + ", " + controlState;
                QStringList contentAtoms;
                const auto contentAtomsArray = controlObj["contents"].toArray();
                for (const QJsonValue &atomValue : contentAtomsArray) {
                    QString atomConfigName = atomValue.toString();
                    atomConfigName.replace('-', '_');
                    contentAtoms.append(atomConfigName);
                }

                generateMirrored(contentAtoms, outputStateConfig);
                generateSpacing(contentAtoms, outputStateConfig);
                generatePadding(outputStateConfig);

            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, generate control: " << e.what() << "; " << m_currentAtomInfo;
            }

            // Add the exported atom configuration to the state configuration
            outputControlConfig.insert(controlState, outputStateConfig);
        } catch (std::exception &e) {
            qWarning().nospace().noquote() << "Warning, generate control: " << e.what() << " " << m_currentAtomInfo;
        }

        // Add the control configuration to the global configuration document
        m_outputConfig.insert(controlName.toLower(), outputControlConfig);
    }

    QJsonObject findAtomObject(const QString &path, const QString &figmaState, const QJsonObject &componentSet)
    {
        // Construct the json search path to the atom
        // inside the component set, and fetch it.
        QStringList jsonPath;
        QStringList atomPath;
        if (!path.isEmpty())
            atomPath = path.split(",");

        jsonPath.append("state=" + figmaState);
        for (const QString &child : atomPath)
            jsonPath += child.trimmed();

        return findNamedChild(jsonPath, componentSet);
    }

    void exportGeometry(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const auto geometry = getGeometry(atom);
        const auto stretch = getStretch(atom);

        outputConfig.insert("x", geometry.x());
        outputConfig.insert("y", geometry.y());
        outputConfig.insert("width", geometry.width());
        outputConfig.insert("height", geometry.height());

        outputConfig.insert("leftOffset", stretch.left());
        outputConfig.insert("topOffset", stretch.top());
        outputConfig.insert("rightOffset", stretch.right());
        outputConfig.insert("bottomOffset", stretch.bottom());

        // Todo: resolve insets for rectangles with drop shadow
        // config.insert("leftInset", 0);
        // config.insert("topInset", 0);
        // config.insert("rightInset", 0);
        // config.insert("bottomInset", 0);
    }

    void exportImage(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const QString figmaId = getString("figmaId", outputConfig);
        const QString imageName = getString("name", outputConfig);

        const QJsonValue visible = atom.value("visible");
        if (visible != QJsonValue::Undefined && !visible.toBool()) {
            // Figma will not generate an image for a hidden child
            debug("skipping hidden image: " + imageName);
            return;
        }

        debug("export image: " + imageName);
        m_imagesToDownload.insert(figmaId, imageName);
        outputConfig.insert("export", "image");
    }

    void exportJson(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const QString name = getString("name", outputConfig);
        const QString fileName = "json/" + name + ".json";
        debug("export json: " + m_currentAtomInfo + "; filename: " + fileName);
        createTextFileInStylefolder(fileName, QJsonDocument(atom).toJson());
    }

    void exportLayout(const QJsonObject &atom , QJsonObject &outputConfig)
    {
        outputConfig.insert("layoutMode", atom["layoutMode"]);
        outputConfig.insert("leftPadding", atom["paddingLeft"]);
        outputConfig.insert("topPadding", atom["paddingTop"]);
        outputConfig.insert("rightPadding", atom["paddingRight"]);
        outputConfig.insert("bottomPadding", atom["paddingBottom"]);
        outputConfig.insert("alignItems", atom["primaryAxisAlignItems"]);
        outputConfig.insert("spacing", atom["itemSpacing"]);
    }

    void generateMirrored(const QStringList &contentAtoms, QJsonObject &outputConfig)
    {
        if (contentAtoms.size() < 2)
            return;

        const QRectF atom0Geo = getConfigGeometry(contentAtoms[0].trimmed(), outputConfig);
        const QRectF atom1Geo = getConfigGeometry(contentAtoms[1].trimmed(), outputConfig);

        // Note that the order in which the content atoms are listed in
        // the config file matters when we now try to calculate if the
        // control is mirrored in the design.
        const bool mirrored = !atom0Geo.isEmpty() && !atom1Geo.isEmpty() && atom0Geo.x() > atom1Geo.x();
        outputConfig.insert("mirrored", mirrored);
    }

    void generateSpacing(const QStringList &contentAtoms, QJsonObject &outputConfig)
    {
        // 'spacing' for a Control tells the exact distance between the items inside
        // the contentItem (typically the label and indicator). Since Figma implements
        // spacing a bit differently (and supports modes such as SPACE_BETWEEN, which
        // Controls don't support), we calculate the spacing ourselves based on the
        // geometry of the content atoms.
        if (contentAtoms.size() < 2)
            return;

        const bool mirrored = outputConfig["mirrored"].toBool();
        const QRectF atom0Geo = getConfigGeometry(contentAtoms[0].trimmed(), outputConfig);
        const QRectF atom1Geo = getConfigGeometry(contentAtoms[1].trimmed(), outputConfig);
        const qreal spacing = mirrored ?
            atom0Geo.x() - atom1Geo.x() - atom1Geo.width() :
            atom1Geo.x() - atom0Geo.x() - atom0Geo.width();

        outputConfig.insert("spacing", spacing);
    }

    void generatePadding(QJsonObject &outputConfig)
    {
        // To be able to generate padding, we require that the layout
        // of an atom 'contentItem' has been exported
        const QJsonValue contentItemValue = outputConfig.value("contentItem");
        if (contentItemValue.isUndefined())
            return;

        const QJsonObject contentItem = contentItemValue.toObject();
        outputConfig.insert("leftPadding", contentItem["leftPadding"]);
        outputConfig.insert("topPadding", contentItem["topPadding"]);
        outputConfig.insert("rightPadding", contentItem["rightPadding"]);
        outputConfig.insert("bottomPadding", contentItem["bottomPadding"]);
    }

    void generateConfiguration()
    {
        QJsonObject root;
        root.insert("version", "1.0");
        root.insert("controls", m_outputConfig);
        debug("generating config.json");
        createTextFileInStylefolder("config.json", QJsonDocument(root).toJson());
    }

    void generateQmlDir()
    {
        const QString styleName = QFileInfo(m_targetPath).fileName();
        const QString version(" 1.0 ");

        QString qmldir;
        qmldir += "module " + styleName + "\n";
        for (const QString &control : m_qmlDirControls)
            qmldir += control + version + control + ".qml\n";

        debug("generating qmldir");
        createTextFileInStylefolder("qmldir", qmldir);
    }

    void mkTargetPath(const QString &path) const
    {
        const QFileInfo fileInfo(path);
        if (fileInfo.exists())
            return;

        const QDir dir = fileInfo.absoluteDir();
        if (!QDir().mkpath(dir.path()))
            throw std::runtime_error("Could not create target path: " + dir.path().toStdString());
    }

    /**
     * Copies the given file into the style folder.
     * If destFileName is empty, the file name of the src will be used.
     */
    void copyFileToStyleFolder(const QString &srcPath, bool overwrite = true, QString destPath = "") const
    {
        QFile srcFile = QFile(srcPath);
        if (!srcFile.exists())
            throw std::runtime_error("File doesn't exist: " + srcPath.toStdString());
        if (destPath.isEmpty())
            destPath = QFileInfo(srcFile).fileName();

        QString targetPath = m_targetPath + "/" + destPath;
        mkTargetPath(targetPath);
        if (!overwrite && QFileInfo(targetPath).exists()) {
            debug(targetPath + " exists, skipping overwrite");
            return;
        }

        debug("copying " + QFileInfo(srcPath).fileName() + " to " + targetPath);
        srcFile.copy(targetPath);

        // Files we copy from resources are read-only, so change target permission
        // so that the user can modify generated QML files etc.
        QFile::setPermissions(targetPath, QFileDevice::ReadOwner | QFileDevice::ReadUser
            | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner);
    }

    void createTextFileInStylefolder(const QString &fileName, const QString &contents) const
    {
        const QString targetPath = m_targetPath + "/" + fileName;
        mkTargetPath(targetPath);
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly))
            throw std::runtime_error("Could not open file for writing: " + targetPath.toStdString());

        QTextStream out(&file);
        out << contents;
    }

    QRectF getGeometry(const QJsonObject figmaObject) const
    {
        const auto bb = getObject("absoluteBoundingBox", figmaObject);
        const auto x = getValue("x", bb).toDouble();
        const auto y = getValue("y", bb).toDouble();
        const auto width = getValue("width", bb).toDouble();
        const auto height = getValue("height", bb).toDouble();
        return QRectF(x, y, width, height);
    }

    QRectF getConfigGeometry(const QString &atomName, const QJsonObject &outputConfig) const
    {
        // Read back the geometry we have already generated in the config object
        const QJsonObject atomObj = getObject(atomName, outputConfig);
        const auto x = getValue("x", atomObj).toDouble();
        const auto y = getValue("y", atomObj).toDouble();
        const auto width = getValue("width", atomObj).toDouble();
        const auto height = getValue("height", atomObj).toDouble();
        return QRectF(x, y, width, height);
    }

    QMargins getStretch(const QJsonObject rectangle) const
    {
        // Determine the stretch factor of the rectangle based on its corner radii
        qreal topLeftRadius = 0;
        qreal topRightRadius = 0;
        qreal bottomRightRadius = 0;
        qreal bottomLeftRadius = 0;

        const QJsonValue radiusValue = rectangle.value("cornerRadius");
        if (radiusValue != QJsonValue::Undefined) {
            const qreal r = radiusValue.toDouble();
            topLeftRadius = r;
            topRightRadius = r;
            bottomRightRadius = r;
            bottomLeftRadius = r;
        } else {
            const QJsonValue radiiValue = rectangle.value("rectangleCornerRadii");
            if (radiiValue == QJsonValue::Undefined)
                return {};
            const QJsonArray r = radiiValue.toArray();
            Q_ASSERT(r.count() == 4);
            topLeftRadius= r[0].toDouble();
            topRightRadius= r[1].toDouble();
            bottomRightRadius = r[2].toDouble();
            bottomLeftRadius = r[3].toDouble();
        }

        const qreal left = qMax(topLeftRadius, bottomLeftRadius);
        const qreal right = qMax(topRightRadius, bottomRightRadius);
        const qreal top = qMax(topLeftRadius, topRightRadius);
        const qreal bottom = qMax(bottomLeftRadius, bottomRightRadius);

        return QMargins(left, top, right, bottom);
    }

    QString networkErrorString(QNetworkReply *reply)
    {
        return QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(reply->error());
    }

    void newProgress(const QString &label)
    {
        if (m_verbose || m_silent)
            return;
        m_progressLabel = label;
        m_progress = 0;
        printf("\33[2K%s: 0\r", qPrintable(m_progressLabel));
        fflush(stdout);
    }

    void progress()
    {
        if (m_verbose || m_silent)
            return;
        printf("\33[2K%s: %i\r", qPrintable(m_progressLabel), ++m_progress);
        fflush(stdout);
    }

    void debug(const QString &msg) const
    {
        if (!m_verbose)
            return;
        qDebug().noquote() << msg;
    }

    void debugHeader(const QString &msg) const
    {
        debug("");
        debug("*** " + msg + " ***");
    }

    void saveForDebug(const QJsonObject &object, const QString &name = "debug.json") const
    {
        debug("saving json for debug: " + name);
        createTextFileInStylefolder(name, QJsonDocument(object).toJson());
    }

private:
    QString m_fileId;
    QString m_token;
    QString m_targetPath;
    bool m_verbose = false;
    bool m_silent = false;

    QStringList m_defaultExport;

    QJsonDocument m_document;
    QJsonObject m_outputConfig;
    QStringList m_qmlDirControls;
    QMap<QString, QString> m_imagesToDownload;

    QString m_currentAtomInfo;

    QString m_progressLabel;
    int m_progress = 0;
};

#endif // QSTYLEREADER_H

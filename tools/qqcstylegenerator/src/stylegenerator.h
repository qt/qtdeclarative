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
        m_currentControl = controlName;
        debugHeader(controlName);

        // info from config document
        const auto configAtoms = getArray("atoms", controlObj);
        const auto componentSetName = getString("component set", controlObj);

        // info from figma document
        const auto documentRoot = getObject("document", m_document.object());
        const auto componentSet = findChild({"type", "COMPONENT_SET", "name", componentSetName}, documentRoot);

        const auto copy = controlObj["copy"].toString();
        QStringList files = copy.split(',');
        for (const QString &file : files)
            copyFileToStyleFolder(file.trimmed(), false);

        // We use content atoms to figure out properties such as spacing and mirrored
        QStringList contentAtoms;
        const auto contentAtomsArray = controlObj["contents"].toArray();
        for (const QJsonValue &atomValue : contentAtomsArray)
            contentAtoms.append(atomValue.toString());

        // Add this control to the list of controls that goes into the qmldir file
        m_qmlDirControls.append(controlName);

        // Search for contentItem below, and process it last, so that we can
        // make use of the other atoms in the calculations while doing so.
        QJsonObject contentItemObj;

        const auto configStatesArray = getArray("states", controlObj);
        for (const QJsonValue &configStateValue : configStatesArray) try {
            // Resolve all atoms for the given state
            contentItemObj = QJsonObject();
            m_currentAtomInfo = "control: " + controlName;
            const QJsonObject configStateObj = configStateValue.toObject();
            const QString controlState = getString("state", configStateObj);
            const QString figmaState = getString("figmaState", configStateObj);

            for (const QJsonValue &atomConfigValue : configAtoms) try {
                m_currentQualifiedAtomName = "";
                m_currentAtomInfo = "control: " + controlName + ", " + controlState;
                const QJsonObject atomConfigObj = atomConfigValue.toObject();
                const QString atomName = getString("atom", atomConfigObj);
                m_currentQualifiedAtomName = qualifiedAtomName(atomName, controlState);
                m_currentAtomInfo = "atom: " + m_currentQualifiedAtomName;
                const auto figmaPath = getString("figmaPath", atomConfigObj);
                m_currentAtomInfo += "; figma path: " + figmaPath;
                const auto atomExport = getAtomExport(atomConfigObj);
                const auto figmaAtomObj = findAtomObject(figmaPath, figmaState, componentSet);
                const auto figmaId = getString("id", figmaAtomObj);
                m_currentAtomInfo += "; id: " + figmaId;

                if (atomName == "contentItem")
                    contentItemObj = figmaAtomObj;

                generateAtomAssets(atomExport, figmaAtomObj);
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, generate atom: " << e.what() << " " << m_currentAtomInfo;
            }

            if (!contentItemObj.isEmpty()) try {
                generatePaddingAndSpacing(contentItemObj, contentAtoms, controlState);
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, generate padding: " << e.what() << " " << m_currentAtomInfo;
            }
        } catch (std::exception &e) {
            qWarning().nospace().noquote() << "Warning, generate control: " << e.what() << " " << m_currentAtomInfo;
        }
    }

    QStringList getAtomExport(const QJsonObject &atomObj)
    {
        const QJsonValue exportValue = atomObj["export"];
        if (!exportValue.isUndefined() && !exportValue.isArray())
            throw std::runtime_error("export is not an array!");

        const QJsonArray exportArray = exportValue.toArray();
        if (exportArray.isEmpty())
            return m_defaultExport;

        QStringList exportList;
        for (const QJsonValue &exportValue : exportArray)
            exportList.append(exportValue.toString());
        if (exportList.isEmpty())
            throw std::runtime_error("missing export config!");

        return exportList;
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

    void generateAtomAssets(const QStringList &atomExportList, const QJsonObject &figmaAtomObj)
    {
        QJsonObject atomOutputConfig;
        atomOutputConfig.insert("figmaId", getString("id", figmaAtomObj));

        for (const QString &atomExport : atomExportList) {
            try {
                if (atomExport == "geometry")
                    generateGeometry(figmaAtomObj, atomOutputConfig);
                else if (atomExport == "image")
                    generateImage(figmaAtomObj, atomOutputConfig);
                else if (atomExport == "json")
                    generateJson(figmaAtomObj, atomOutputConfig);
                else
                    throw std::runtime_error("Unknown option: '" + atomExport.toStdString() + "'");
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, export atom: " << e.what() << " " << m_currentAtomInfo;
            }
        }

        // Add the atom configuration to the global config document
        m_outputConfig.insert(m_currentQualifiedAtomName, atomOutputConfig);
    }

    void generateGeometry(const QJsonObject &atom, QJsonObject &outputConfig)
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

    void generateImage(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const QString figmaId = getString("id", atom);
        const QJsonValue visible = atom.value("visible");

        if (visible != QJsonValue::Undefined && !visible.toBool()) {
            // Figma will not generate an image for a hidden child
            debug("skipping hidden image: " + m_currentAtomInfo);
            return;
        }

        debug("export image: " + m_currentAtomInfo);
        m_imagesToDownload.insert(figmaId, m_currentQualifiedAtomName);
        addExportType("image", outputConfig);
    }

    void generateJson(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const QString figmaId = getString("id", atom);
        const QString fileName = "json/" + m_currentQualifiedAtomName + ".json";
        debug("export json: " + m_currentAtomInfo + "; filename: " + fileName);
        createTextFileInStylefolder(fileName, QJsonDocument(atom).toJson());
        addExportType("json", outputConfig);
    }

    void generatePaddingAndSpacing(const QJsonObject &contentItemObj, const QStringList &contentAtoms, const QString &imageState)
    {
        // We expect there to be a "contentItem" atom that points to node
        // that contain all the content atoms. And for now we assume that the
        // contentItem always uses "auto layout" in Figma, so that we can read
        // out all the padding information directly from the design.

        const bool hasHorizontalLayout = contentItemObj["layoutMode"].toString() == "HORIZONTAL";
        if (!hasHorizontalLayout)
            throw std::runtime_error("the contentItem needs to a use horizontal auto layout!");

        // Store padding and spacing info inside the background atom config
        const QString bgQualifiedName = qualifiedAtomName("background", imageState);
        QJsonObject bgConfig = getConfigObject(bgQualifiedName);

        // Resolve the geometries of the content item atoms
        QRectF atom0Geo, atom1Geo;
        if (contentAtoms.size() >= 2) {
            const QString atom0Name = contentAtoms[0].trimmed();
            const QString atom1Name = contentAtoms[1].trimmed();
            const QString atom0QualifiedName = qualifiedAtomName(atom0Name, imageState);
            const QString atom1QualifiedName = qualifiedAtomName(atom1Name, imageState);
            const QJsonObject atom0Obj = getConfigObject(atom0QualifiedName);
            const QJsonObject atom1Obj = getConfigObject(atom1QualifiedName);
            atom0Geo = getConfigGeometry(atom0Obj);
            atom1Geo = getConfigGeometry(atom1Obj);
        }

        // Note that the order in which the content atoms are listed in
        // the config file matters when we now try to calculate if the
        // control is mirrored in the design.
        const bool mirrored = !atom0Geo.isEmpty() && !atom1Geo.isEmpty() && atom0Geo.x() > atom1Geo.x();

        qreal spacing = 0;
        const QString sizingMode = contentItemObj["primaryAxisAlignItems"].toString();
        if (sizingMode == "SPACE_BETWEEN") {
            try {
                // SPACE_BETWEEN means spread the atoms evenly in the available space (and
                // align left-most atom to the left, and the right-most atom to the right).
                // But since we currently don't support that way of aligning the atoms in
                // our controls templates, we try to calculate a fixed spacing instead by
                // looking at the children geometries.
                spacing = mirrored ?
                    atom0Geo.x() - atom1Geo.x() - atom1Geo.width() :
                    atom1Geo.x() - atom0Geo.x() - atom0Geo.width();
            } catch (std::exception &e) {
                qWarning().noquote() << "Warning! " << m_currentAtomInfo << "; Could not calculate spacing:" << e.what();
            }
        } else {
            spacing = contentItemObj["itemSpacing"].toDouble();
        }

        bgConfig.insert("spacing", spacing);
        bgConfig.insert("mirrored", mirrored);
        bgConfig.insert("leftPadding", contentItemObj["paddingLeft"]);
        bgConfig.insert("topPadding", contentItemObj["paddingTop"]);
        bgConfig.insert("rightPadding", contentItemObj["paddingRight"]);
        bgConfig.insert("bottomPadding", contentItemObj["paddingBottom"]);

        // Overwrite the previous config
        m_outputConfig.insert(bgQualifiedName, bgConfig);
    }

    void generateConfiguration()
    {
        QJsonObject root;
        root.insert("version", "1.0");
        root.insert("atoms", m_outputConfig);
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

    void addExportType(const QString &type, QJsonObject &outputConfig)
    {
        QString exportTypes = outputConfig.value("export").toString();
        if (!exportTypes.isEmpty())
            exportTypes += ",";
        outputConfig.insert("export", exportTypes + type);
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

    QString qualifiedAtomName(const QString &atomName, const QString &imageState)
    {
        // The "qualified" name is the complete name of the atom
        // which is equal to the name of the exported image (if any), and
        // also the key/name of the atom in the config.json file we produce.
        const QString stateString = imageState == "normal" ? "" : "-" + imageState;
        return m_currentControl.toLower() + "-" + atomName + stateString;
    }

    QJsonObject getConfigObject(const QString &qualifiedName)
    {
        // Read back the atom configuration from the already generated config object
        QJsonObject config = m_outputConfig[qualifiedName].toObject();
        if (config.isEmpty())
            throw std::runtime_error("atom config not found: " + qualifiedName.toStdString());
        return config;
    }

    QRectF getConfigGeometry(const QJsonObject configAtom) const
    {
        // Read back the geometry we have already generated in the config object
        const auto x = getValue("x", configAtom).toDouble();
        const auto y = getValue("y", configAtom).toDouble();
        const auto width = getValue("width", configAtom).toDouble();
        const auto height = getValue("height", configAtom).toDouble();
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

    QString m_currentControl;
    QString m_currentQualifiedAtomName;
    QString m_currentAtomInfo;

    QString m_progressLabel;
    int m_progress = 0;
};

#endif // QSTYLEREADER_H
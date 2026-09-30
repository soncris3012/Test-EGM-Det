#pragma once
#include "types.h"
#include <QDir>

struct Sample { QString id, rgbPath, irPath, annotationPath; };

class DataLoader {
public:
    explicit DataLoader(QString root);
    QList<Sample> discover(QString *error) const;
    static QList<OrientedBox> loadAnnotation(const QString &path, const QSize &imageSize, const QString &imageId);
private:
    QString m_root;
};

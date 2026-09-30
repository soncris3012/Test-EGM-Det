#pragma once

#include <QImage>
#include <QList>
#include <QMetaType>
#include <QPolygonF>
#include <QString>
#include <QVector>

struct OrientedBox {
    QString className;
    QPointF center;
    QSizeF size;
    double angleDegrees = 0.0;
    double confidence = 1.0;
    bool groundTruth = false;
    QString imageId;
};

struct ClassMetrics {
    QString className;
    int targets = 0;
    int detections = 0;
    double precision = 0.0;
    double recall = 0.0;
    double ap50 = 0.0;
    double ap75 = 0.0;
    double map5095 = 0.0;
};

struct BenchmarkReport {
    QList<ClassMetrics> rows;
    int processed = 0;
    int total = 0;
    double elapsedMs = 0.0;
    double rgbPreferencePct = 0.0;
    double irPreferencePct = 0.0;
    double ambiguousPct = 0.0;
    double meanEntropy = 0.0;
    bool reliabilityOnly = false;
};

struct FrameResult {
    QImage rgb;
    QImage ir;
    QImage gateMap;
    QList<OrientedBox> boxes;
    QString fileName;
    double preprocessMs = 0.0;
    double forwardMs = 0.0;
    double nmsMs = 0.0;
    double rgbPreferencePct = 0.0;
    double irPreferencePct = 0.0;
    double ambiguousPct = 0.0;
    double meanEntropy = 0.0;
};

struct BenchmarkConfig {
    QString datasetPath;
    QString task;
    QString modelPath;
    double confidence = 0.25;
    double nmsIou = 0.45;
    int batchSize = 1;
    int inputSize = 640;
    bool singlePair = false;
    bool showGate = false;
};

Q_DECLARE_METATYPE(OrientedBox)
Q_DECLARE_METATYPE(QList<OrientedBox>)
Q_DECLARE_METATYPE(FrameResult)
Q_DECLARE_METATYPE(BenchmarkReport)
Q_DECLARE_METATYPE(BenchmarkConfig)

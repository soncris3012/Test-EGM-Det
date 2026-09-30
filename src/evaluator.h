#pragma once
#include "types.h"

class Evaluator {
public:
    void clear();
    void addGroundTruth(const OrientedBox &box);
    void addDetection(const OrientedBox &box);
    BenchmarkReport evaluate() const;
    static double rotatedIoU(const OrientedBox &a, const OrientedBox &b);

private:
    QList<OrientedBox> m_groundTruth;
    QList<OrientedBox> m_detections;
    static double averagePrecision(const QList<OrientedBox> &gt, const QList<OrientedBox> &det, double threshold);
};

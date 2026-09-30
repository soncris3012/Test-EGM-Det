#pragma once
#include <QImage>

struct ReliabilityResult {
    QImage gateMap;
    double rgbPreferencePct = 0.0;
    double irPreferencePct = 0.0;
    double ambiguousPct = 0.0;
    double meanEntropy = 0.0;
};

class ReliabilityAnalyzer {
public:
    static ReliabilityResult analyze(const QImage &rgb, const QImage &ir);
};

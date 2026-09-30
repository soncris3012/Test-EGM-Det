#pragma once
#include "types.h"
#include <QObject>
#include <atomic>

class BenchmarkWorker : public QObject {
    Q_OBJECT
public:
    explicit BenchmarkWorker(QObject *parent=nullptr);
public Q_SLOTS:
    void run(BenchmarkConfig config);
    void cancel();
Q_SIGNALS:
    void progressChanged(int current,int total);
    void frameProcessed(FrameResult result);
    void benchmarkFinished(BenchmarkReport report);
    void failed(QString message);
private:
    std::atomic_bool m_cancelled{false};
};

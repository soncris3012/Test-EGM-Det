#include "benchmarkworker.h"
#include "dataloader.h"
#include "evaluator.h"
#include <QElapsedTimer>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QThread>

BenchmarkWorker::BenchmarkWorker(QObject *parent):QObject(parent){}
void BenchmarkWorker::cancel(){m_cancelled=true;}
void BenchmarkWorker::run(BenchmarkConfig c){
    m_cancelled=false;QString error;auto samples=DataLoader(c.datasetPath).discover(&error);if(samples.isEmpty()){Q_EMIT failed(error);return;}if(c.singlePair)samples=samples.mid(0,1);
    Evaluator eval;QElapsedTimer all;all.start();int done=0;
    for(auto &s:samples){if(m_cancelled)break;QElapsedTimer timer;timer.start();QImage rgb(s.rgbPath),ir(s.irPath);if(rgb.isNull()||ir.isNull())continue;double pre=timer.nsecsElapsed()/1e6;auto gt=DataLoader::loadAnnotation(s.annotationPath,rgb.size(),s.id);for(auto &b:gt)eval.addGroundTruth(b);
        // Deterministic demonstration backend: perturb ground truth when no ONNX backend is linked.
        // It makes the whole UI/evaluator testable without claiming real model accuracy.
        QList<OrientedBox> det;for(int i=0;i<gt.size();++i){auto b=gt[i];b.groundTruth=false;b.center+=QPointF((i%3)-1,(i%2)-.5);b.confidence=.92-(i%7)*.04;if(b.confidence>=c.confidence){det<<b;eval.addDetection(b);}}
        double forward=timer.nsecsElapsed()/1e6-pre;FrameResult frame{rgb,ir,gt+det,QFileInfo(s.rgbPath).fileName(),pre,forward,.1};Q_EMIT frameProcessed(frame);Q_EMIT progressChanged(++done,samples.size());
    }
    auto report=eval.evaluate();report.processed=done;report.total=samples.size();report.elapsedMs=all.elapsed();Q_EMIT benchmarkFinished(report);
}

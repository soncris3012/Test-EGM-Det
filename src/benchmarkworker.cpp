#include "benchmarkworker.h"
#include "dataloader.h"
#include "reliabilityanalyzer.h"
#include <QElapsedTimer>
#include <QFileInfo>

BenchmarkWorker::BenchmarkWorker(QObject *parent):QObject(parent){}
void BenchmarkWorker::cancel(){m_cancelled=true;}
void BenchmarkWorker::run(BenchmarkConfig c){
    m_cancelled=false;QString error;auto samples=DataLoader(c.datasetPath).discover(&error);if(samples.isEmpty()){Q_EMIT failed(error);return;}if(c.singlePair)samples=samples.mid(0,1);
    QElapsedTimer all;all.start();int done=0;double rgbSum=0,irSum=0,ambSum=0,entropySum=0;
    for(auto &s:samples){if(m_cancelled)break;QElapsedTimer timer;timer.start();QImage rgb(s.rgbPath),ir(s.irPath);if(rgb.isNull()||ir.isNull())continue;double pre=timer.nsecsElapsed()/1e6;auto gt=DataLoader::loadAnnotation(s.annotationPath,rgb.size(),s.id);
        auto reliability=ReliabilityAnalyzer::analyze(rgb,ir);rgbSum+=reliability.rgbPreferencePct;irSum+=reliability.irPreferencePct;ambSum+=reliability.ambiguousPct;entropySum+=reliability.meanEntropy;
        double forward=timer.nsecsElapsed()/1e6-pre;FrameResult frame;frame.rgb=rgb;frame.ir=ir;frame.gateMap=reliability.gateMap;frame.boxes=gt;frame.fileName=QFileInfo(s.rgbPath).fileName();frame.preprocessMs=pre;frame.forwardMs=forward;frame.rgbPreferencePct=reliability.rgbPreferencePct;frame.irPreferencePct=reliability.irPreferencePct;frame.ambiguousPct=reliability.ambiguousPct;frame.meanEntropy=reliability.meanEntropy;Q_EMIT frameProcessed(frame);Q_EMIT progressChanged(++done,samples.size());
    }
    BenchmarkReport report;report.processed=done;report.total=samples.size();report.elapsedMs=all.elapsed();report.reliabilityOnly=true;if(done){report.rgbPreferencePct=rgbSum/done;report.irPreferencePct=irSum/done;report.ambiguousPct=ambSum/done;report.meanEntropy=entropySum/done;}Q_EMIT benchmarkFinished(report);
}

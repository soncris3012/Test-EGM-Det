#include "benchmarkworker.h"
#include "dataloader.h"
#include "reliabilityanalyzer.h"
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFuture>
#include <QThread>
#include <QtConcurrentRun>
#include <algorithm>

namespace {
struct ProcessedSample { FrameResult frame; bool valid=false; };
ProcessedSample processSample(const Sample &s){QElapsedTimer timer;timer.start();QImage rgb(s.rgbPath),ir(s.irPath);if(rgb.isNull()||ir.isNull())return {};double pre=timer.nsecsElapsed()/1e6;auto gt=DataLoader::loadAnnotation(s.annotationPath,rgb.size(),s.id);auto reliability=ReliabilityAnalyzer::analyze(rgb,ir);ProcessedSample out;out.valid=true;auto &f=out.frame;f.rgb=rgb;f.ir=ir;f.gateMap=reliability.gateMap;f.boxes=gt;f.fileName=QFileInfo(s.rgbPath).fileName();f.preprocessMs=pre;f.forwardMs=timer.nsecsElapsed()/1e6-pre;f.rgbPreferencePct=reliability.rgbPreferencePct;f.irPreferencePct=reliability.irPreferencePct;f.ambiguousPct=reliability.ambiguousPct;f.meanEntropy=reliability.meanEntropy;return out;}
}

BenchmarkWorker::BenchmarkWorker(QObject *parent):QObject(parent){}
void BenchmarkWorker::cancel(){m_cancelled=true;}
void BenchmarkWorker::run(BenchmarkConfig c){
    m_cancelled=false;QString error;auto samples=DataLoader(c.datasetPath).discover(&error);if(samples.isEmpty()){Q_EMIT failed(error);return;}const int discoveredTotal=int(samples.size());int selectedIndex=0;if(c.singlePair){selectedIndex=((c.sampleIndex%discoveredTotal)+discoveredTotal)%discoveredTotal;samples=samples.mid(selectedIndex,1);}
    QElapsedTimer all;all.start();int done=0;double rgbSum=0,irSum=0,ambSum=0,entropySum=0;const int total=int(samples.size()),automatic=std::clamp(QThread::idealThreadCount(),2,8),parallelism=c.batchSize>1?std::clamp(c.batchSize,2,16):automatic,previewStep=std::max(1,total/100);
    BenchmarkReport report;report.reliabilityOnly=true;
    for(int start=0;start<total&&!m_cancelled;start+=parallelism){QList<QFuture<ProcessedSample>> futures;const int end=std::min(start+parallelism,total);for(int i=start;i<end;++i)futures<<QtConcurrent::run([s=samples[i]]{return processSample(s);});for(auto &future:futures){auto result=future.result();if(!result.valid)continue;auto &f=result.frame;f.pairIndex=c.singlePair?selectedIndex:done;f.pairTotal=discoveredTotal;rgbSum+=f.rgbPreferencePct;irSum+=f.irPreferencePct;ambSum+=f.ambiguousPct;entropySum+=f.meanEntropy;report.imageRows<<ImageReliabilityMetrics{f.fileName,f.rgbPreferencePct,f.irPreferencePct,f.ambiguousPct,f.meanEntropy,f.preprocessMs+f.forwardMs};++done;if(done==1||done==total||done%previewStep==0)Q_EMIT frameProcessed(f);Q_EMIT progressChanged(c.singlePair?selectedIndex+1:done,discoveredTotal);}
    }
    report.processed=done;report.total=c.singlePair?discoveredTotal:samples.size();report.elapsedMs=all.elapsed();if(done){report.rgbPreferencePct=rgbSum/done;report.irPreferencePct=irSum/done;report.ambiguousPct=ambSum/done;report.meanEntropy=entropySum/done;}Q_EMIT benchmarkFinished(report);
}

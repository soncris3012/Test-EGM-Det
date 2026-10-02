#include "../src/benchmarkworker.h"
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <iostream>

int main(int argc,char **argv){QCoreApplication app(argc,argv);QTemporaryDir temp;QDir root(temp.path());root.mkpath("visible/test");root.mkpath("infrared/test");QImage image(16,16,QImage::Format_RGB32);image.fill(Qt::gray);for(const auto &name:{"0001.jpg","0002.jpg"}){image.save(root.filePath("visible/test/"+QString(name)));image.save(root.filePath("infrared/test/"+QString(name)));}BenchmarkWorker worker;FrameResult received;BenchmarkReport report;QObject::connect(&worker,&BenchmarkWorker::frameProcessed,[&](FrameResult f){received=f;});QObject::connect(&worker,&BenchmarkWorker::benchmarkFinished,[&](BenchmarkReport r){report=r;});BenchmarkConfig config;config.datasetPath=temp.path();config.singlePair=true;config.sampleIndex=1;worker.run(config);if(received.fileName!="0002.jpg"||received.pairIndex!=1||received.pairTotal!=2){std::cerr<<"single-pair navigation selected the wrong pair\n";return 1;}if(report.imageRows.size()!=1||report.imageRows[0].fileName!="0002.jpg"){std::cerr<<"per-image report row missing\n";return 2;}config.singlePair=false;worker.run(config);if(report.imageRows.size()!=2||report.processed!=2){std::cerr<<"batch report must contain every image row\n";return 3;}std::cout<<"Single-pair navigation and per-image batch report passed\n";return 0;}

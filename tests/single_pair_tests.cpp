#include "../src/benchmarkworker.h"
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <iostream>

int main(int argc,char **argv){QCoreApplication app(argc,argv);QTemporaryDir temp;QDir root(temp.path());root.mkpath("visible/test");root.mkpath("infrared/test");QImage image(16,16,QImage::Format_RGB32);image.fill(Qt::gray);for(const auto &name:{"0001.jpg","0002.jpg"}){image.save(root.filePath("visible/test/"+QString(name)));image.save(root.filePath("infrared/test/"+QString(name)));}BenchmarkWorker worker;FrameResult received;QObject::connect(&worker,&BenchmarkWorker::frameProcessed,[&](FrameResult f){received=f;});BenchmarkConfig config;config.datasetPath=temp.path();config.singlePair=true;config.sampleIndex=1;worker.run(config);if(received.fileName!="0002.jpg"||received.pairIndex!=1||received.pairTotal!=2){std::cerr<<"single-pair navigation selected the wrong pair\n";return 1;}std::cout<<"Single-pair navigation passed\n";return 0;}

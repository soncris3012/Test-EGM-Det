#include "../src/dataloader.h"
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <iostream>

int main(int argc,char **argv){QCoreApplication app(argc,argv);QTemporaryDir temp;if(!temp.isValid())return 1;QDir root(temp.path());root.mkpath("visible/test");root.mkpath("infrared/test");root.mkpath("visible/train");root.mkpath("infrared/train");QImage image(8,8,QImage::Format_RGB32);image.fill(Qt::gray);image.save(root.filePath("visible/test/190001.jpg"));image.save(root.filePath("infrared/test/190001.jpg"));image.save(root.filePath("visible/train/010001.jpg"));image.save(root.filePath("infrared/train/010001.jpg"));QString error;auto samples=DataLoader(temp.path()).discover(&error);if(samples.size()!=1||samples[0].id!="190001"||!samples[0].rgbPath.contains("visible/test")){std::cerr<<error.toStdString()<<"\n";return 2;}auto fromSplit=DataLoader(root.filePath("infrared/test")).discover(&error);if(fromSplit.size()!=1||fromSplit[0].id!="190001"){std::cerr<<"split-folder selection failed\n";return 3;}QTemporaryDir road;QDir rr(road.path());rr.mkpath("crop_HR_visible");rr.mkpath("cropinfrared");image.save(rr.filePath("crop_HR_visible/FLIR_00001.jpg"));image.save(rr.filePath("cropinfrared/FLIR_00001.jpg"));auto roadSamples=DataLoader(road.path()).discover(&error);if(roadSamples.size()!=1||roadSamples[0].id!="FLIR_00001"){std::cerr<<"RoadScene layout failed: "<<error.toStdString()<<"\n";return 4;}std::cout<<"LLVIP and RoadScene layout discovery passed\n";return 0;}

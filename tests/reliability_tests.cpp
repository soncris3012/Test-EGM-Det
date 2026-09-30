#include "../src/reliabilityanalyzer.h"
#include <QCoreApplication>
#include <iostream>

int main(int argc,char **argv){QCoreApplication app(argc,argv);QImage flat(64,64,QImage::Format_RGB32);flat.fill(Qt::black);QImage detail(64,64,QImage::Format_RGB32);for(int y=0;y<64;++y)for(int x=0;x<64;++x)detail.setPixel(x,y,((x+y)&1)?qRgb(255,255,255):qRgb(30,30,30));auto r=ReliabilityAnalyzer::analyze(detail,flat);if(r.gateMap.isNull()||r.rgbPreferencePct<=r.irPreferencePct){std::cerr<<"detailed RGB should be preferred\n";return 1;}if(r.rgbPreferencePct+r.irPreferencePct+r.ambiguousPct<99.9){std::cerr<<"percentages invalid\n";return 2;}std::cout<<"Reliability tests passed\n";return 0;}

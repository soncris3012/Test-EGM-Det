#include "reliabilityanalyzer.h"
#include <QtMath>
#include <algorithm>
#include <array>
#include <cmath>

namespace {
struct Features { QVector<float> score, entropy; int w=0,h=0; };

Features extract(const QImage &source, const QSize &size) {
    QImage gray=source.convertToFormat(QImage::Format_Grayscale8).scaled(size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    Features f;f.w=gray.width();f.h=gray.height();f.score.resize(f.w*f.h);f.entropy.resize(f.w*f.h);
    const int stride=f.w+1,total=(f.w+1)*(f.h+1);QVector<double> sum(total),sum2(total);std::array<QVector<int>,16> hist;for(auto &h:hist)h.resize(total);
    for(int y=1;y<=f.h;++y){const uchar *line=gray.constScanLine(y-1);double rowSum=0,rowSum2=0;std::array<int,16> rowHist{};for(int x=1;x<=f.w;++x){int v=line[x-1],idx=y*stride+x,above=(y-1)*stride+x;rowSum+=v;rowSum2+=v*v;++rowHist[v>>4];sum[idx]=sum[above]+rowSum;sum2[idx]=sum2[above]+rowSum2;for(int b=0;b<16;++b)hist[b][idx]=hist[b][above]+rowHist[b];}}
    auto rect=[stride](const auto &integral,int x0,int y0,int x1,int y1){int a=y0*stride+x0,b=y0*stride+(x1+1),c=(y1+1)*stride+x0,d=(y1+1)*stride+(x1+1);return integral[d]-integral[b]-integral[c]+integral[a];};
    for(int y=0;y<f.h;++y)for(int x=0;x<f.w;++x){
        int x0=std::max(0,x-2),x1=std::min(f.w-1,x+2),y0=std::max(0,y-2),y1=std::min(f.h-1,y+2),count=(x1-x0+1)*(y1-y0+1);double s=rect(sum,x0,y0,x1,y1),s2=rect(sum2,x0,y0,x1,y1),entropy=0;for(const auto &h:hist){int n=rect(h,x0,y0,x1,y1);if(n){double p=double(n)/count;entropy-=p*std::log2(p);}}double mean=s/count;double variance=std::max(0.0,s2/count-mean*mean);double contrast=std::min(1.0,std::sqrt(variance)/64.0);double exposure=std::clamp(1.0-std::abs(mean-127.5)/127.5,0.12,1.0);int i=y*f.w+x;f.entropy[i]=entropy/4.0;f.score[i]=float((.65*f.entropy[i]+.35*contrast)*exposure);
    }return f;
}
}

ReliabilityResult ReliabilityAnalyzer::analyze(const QImage &rgb,const QImage &ir){
    ReliabilityResult out;if(rgb.isNull()||ir.isNull())return out;QSize work=rgb.size();work.scale(192,192,Qt::KeepAspectRatio);auto r=extract(rgb,work),t=extract(ir,work);QImage map(work,QImage::Format_RGB32);int rp=0,ip=0,amb=0;double ent=0;
    for(int y=0;y<work.height();++y){QRgb *line=reinterpret_cast<QRgb*>(map.scanLine(y));for(int x=0;x<work.width();++x){int i=y*work.width()+x;double delta=(r.score[i]-t.score[i])*7.0;double wr=1.0/(1.0+std::exp(-delta));ent+=(r.entropy[i]+t.entropy[i])*.5;if(std::abs(wr-.5)<.10){++amb;line[x]=qRgb(125,125,125);}else if(wr>.5){++rp;int strength=int(90+165*std::min(1.0,(wr-.5)*2));line[x]=qRgb(40,105,strength);}else{++ip;int strength=int(90+165*std::min(1.0,(.5-wr)*2));line[x]=qRgb(strength,80,35);}}}
    double n=work.width()*work.height();out.rgbPreferencePct=100*rp/n;out.irPreferencePct=100*ip/n;out.ambiguousPct=100*amb/n;out.meanEntropy=ent/n;out.gateMap=map.scaled(rgb.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);return out;
}

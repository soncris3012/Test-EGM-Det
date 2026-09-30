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
    for(int y=0;y<f.h;++y)for(int x=0;x<f.w;++x){
        std::array<int,16> bins{};int count=0;double sum=0,sum2=0;
        for(int yy=std::max(0,y-2);yy<=std::min(f.h-1,y+2);++yy){const uchar *line=gray.constScanLine(yy);for(int xx=std::max(0,x-2);xx<=std::min(f.w-1,x+2);++xx){int v=line[xx];++bins[v>>4];sum+=v;sum2+=v*v;++count;}}
        double entropy=0;for(int n:bins)if(n){double p=double(n)/count;entropy-=p*std::log2(p);}double mean=sum/count;double variance=std::max(0.0,sum2/count-mean*mean);double contrast=std::min(1.0,std::sqrt(variance)/64.0);double exposure=std::clamp(1.0-std::abs(mean-127.5)/127.5,0.12,1.0);int i=y*f.w+x;f.entropy[i]=entropy/4.0;f.score[i]=float((.65*f.entropy[i]+.35*contrast)*exposure);
    }return f;
}
}

ReliabilityResult ReliabilityAnalyzer::analyze(const QImage &rgb,const QImage &ir){
    ReliabilityResult out;if(rgb.isNull()||ir.isNull())return out;QSize work=rgb.size();work.scale(192,192,Qt::KeepAspectRatio);auto r=extract(rgb,work),t=extract(ir,work);QImage map(work,QImage::Format_RGB32);int rp=0,ip=0,amb=0;double ent=0;
    for(int y=0;y<work.height();++y){QRgb *line=reinterpret_cast<QRgb*>(map.scanLine(y));for(int x=0;x<work.width();++x){int i=y*work.width()+x;double delta=(r.score[i]-t.score[i])*7.0;double wr=1.0/(1.0+std::exp(-delta));ent+=(r.entropy[i]+t.entropy[i])*.5;if(std::abs(wr-.5)<.10){++amb;line[x]=qRgb(125,125,125);}else if(wr>.5){++rp;int strength=int(90+165*std::min(1.0,(wr-.5)*2));line[x]=qRgb(40,105,strength);}else{++ip;int strength=int(90+165*std::min(1.0,(.5-wr)*2));line[x]=qRgb(strength,80,35);}}}
    double n=work.width()*work.height();out.rgbPreferencePct=100*rp/n;out.irPreferencePct=100*ip/n;out.ambiguousPct=100*amb/n;out.meanEntropy=ent/n;out.gateMap=map.scaled(rgb.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);return out;
}

#include "evaluator.h"
#include <QLineF>
#include <QPainterPath>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <numeric>

static QPolygonF polygonFor(const OrientedBox &b) {
    const double hw = b.size.width() / 2.0, hh = b.size.height() / 2.0;
    QPolygonF p({{-hw,-hh},{hw,-hh},{hw,hh},{-hw,hh}});
    QTransform t; t.translate(b.center.x(), b.center.y()); t.rotate(b.angleDegrees);
    return t.map(p);
}

void Evaluator::clear() { m_groundTruth.clear(); m_detections.clear(); }
void Evaluator::addGroundTruth(const OrientedBox &b) { m_groundTruth << b; }
void Evaluator::addDetection(const OrientedBox &b) { m_detections << b; }

double Evaluator::rotatedIoU(const OrientedBox &a, const OrientedBox &b) {
    QPainterPath pa, pb; pa.addPolygon(polygonFor(a)); pb.addPolygon(polygonFor(b));
    const double intersection = pa.intersected(pb).toFillPolygon().isEmpty() ? 0.0 : pa.intersected(pb).toFillPolygon().boundingRect().isEmpty() ? 0.0 : std::abs([](const QPolygonF &p){ double s=0; for(int i=0;i<p.size();++i){auto x=p[i],y=p[(i+1)%p.size()];s+=x.x()*y.y()-y.x()*x.y();} return s/2.0; }(pa.intersected(pb).toFillPolygon()));
    const double unionArea = a.size.width()*a.size.height()+b.size.width()*b.size.height()-intersection;
    return unionArea > 0.0 ? intersection/unionArea : 0.0;
}

double Evaluator::averagePrecision(const QList<OrientedBox> &gt, const QList<OrientedBox> &detections, double threshold) {
    if (gt.isEmpty()) return 0.0;
    QList<OrientedBox> det = detections;
    std::sort(det.begin(), det.end(), [](auto &a, auto &b){ return a.confidence > b.confidence; });
    QSet<int> matched; QVector<double> tp, fp;
    for (const auto &d : det) {
        int best=-1; double bestIou=threshold;
        for (int i=0;i<gt.size();++i) if (!matched.contains(i) && gt[i].imageId==d.imageId) {
            const double iou=rotatedIoU(gt[i],d); if(iou>=bestIou){bestIou=iou;best=i;}
        }
        tp << (best>=0 ? 1.0 : 0.0); fp << (best<0 ? 1.0 : 0.0); if(best>=0) matched.insert(best);
    }
    for(int i=1;i<tp.size();++i){tp[i]+=tp[i-1];fp[i]+=fp[i-1];}
    double ap=0.0;
    for(int r=0;r<=100;++r){ double bestP=0.0, recall=r/100.0; for(int i=0;i<tp.size();++i) if(tp[i]/gt.size()>=recall) bestP=std::max(bestP,tp[i]/(tp[i]+fp[i])); ap+=bestP; }
    return ap/101.0;
}

BenchmarkReport Evaluator::evaluate() const {
    BenchmarkReport report; QSet<QString> names;
    for(auto &b:m_groundTruth) names.insert(b.className); for(auto &b:m_detections) names.insert(b.className);
    QStringList sorted(names.begin(),names.end()); sorted.sort(Qt::CaseInsensitive);
    for(const auto &name:sorted){
        QList<OrientedBox> gt,det; for(auto &b:m_groundTruth)if(b.className==name)gt<<b; for(auto &b:m_detections)if(b.className==name)det<<b;
        ClassMetrics m; m.className=name; m.targets=gt.size(); m.detections=det.size();
        int tp=0; QSet<int> used; for(auto &d:det){int best=-1;double v=.5;for(int i=0;i<gt.size();++i)if(!used.contains(i)&&gt[i].imageId==d.imageId){double x=rotatedIoU(gt[i],d);if(x>=v){v=x;best=i;}}if(best>=0){++tp;used.insert(best);}}
        m.precision=det.isEmpty()?0.0:100.0*tp/det.size(); m.recall=gt.isEmpty()?0.0:100.0*tp/gt.size();
        double sum=0; for(int i=0;i<10;++i){double ap=averagePrecision(gt,det,.5+i*.05);if(i==0)m.ap50=ap*100;if(i==5)m.ap75=ap*100;sum+=ap;}
        m.map5095=sum*10.0; report.rows<<m;
    }
    if(!report.rows.isEmpty()){
        ClassMetrics o; o.className="Overall"; for(auto&m:report.rows){o.targets+=m.targets;o.detections+=m.detections;o.precision+=m.precision;o.recall+=m.recall;o.ap50+=m.ap50;o.ap75+=m.ap75;o.map5095+=m.map5095;}
        const double n=report.rows.size();o.precision/=n;o.recall/=n;o.ap50/=n;o.ap75/=n;o.map5095/=n;report.rows<<o;
    }
    return report;
}

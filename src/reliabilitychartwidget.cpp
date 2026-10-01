#include "reliabilitychartwidget.h"
#include <QPainter>
#include <algorithm>

ReliabilityChartWidget::ReliabilityChartWidget(QWidget *parent):QWidget(parent){setMinimumHeight(125);}
void ReliabilityChartWidget::setMetrics(double rgb,double ir,double ambiguous,double entropy){m_rgb=rgb;m_ir=ir;m_ambiguous=ambiguous;m_entropy=entropy;update();}
void ReliabilityChartWidget::paintEvent(QPaintEvent *){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.fillRect(rect(),QColor("#202020"));
    const int left=92,right=58,top=15,bottom=10;QRect area(left,top,width()-left-right,height()-top-bottom);
    const double values[4]={m_rgb,m_ir,m_ambiguous,std::clamp(m_entropy*100.0,0.0,100.0)};const QColor colors[4]={QColor("#3478e5"),QColor("#e76b34"),QColor("#888888"),QColor("#8d63d8")};const QString labels[4]={"RGB reliable","IR reliable","Ambiguous","Entropy"};int gap=7;int bh=std::max(14,(area.height()-gap*5)/4);
    for(int i=0;i<4;++i){int y=area.top()+gap+(bh+gap)*i;p.setPen(QColor("#ddd"));p.drawText(QRect(4,y,82,bh),Qt::AlignVCenter|Qt::AlignRight,labels[i]);QRect track(area.left(),y,area.width(),bh);p.fillRect(track,QColor("#343434"));QRect bar(track.left(),y,int(track.width()*values[i]/100.0),bh);p.fillRect(bar,colors[i]);p.setPen(Qt::white);p.drawText(QRect(track.right()+6,y,50,bh),Qt::AlignVCenter,QString::number(values[i],'f',1)+"%");}
}

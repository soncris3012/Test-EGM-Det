#include "reliabilitychartwidget.h"
#include <QPainter>
#include <algorithm>

ReliabilityChartWidget::ReliabilityChartWidget(QWidget *parent):QWidget(parent){setMinimumWidth(210);setMinimumHeight(250);}
void ReliabilityChartWidget::setMetrics(double rgb,double ir,double ambiguous,double entropy){m_rgb=rgb;m_ir=ir;m_ambiguous=ambiguous;m_entropy=entropy;update();}
void ReliabilityChartWidget::paintEvent(QPaintEvent *){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.fillRect(rect(),QColor("#202020"));
    const int left=34,right=10,top=36,bottom=54;QRect area(left,top,width()-left-right,height()-top-bottom);p.setPen(QColor("#888"));p.drawLine(area.bottomLeft(),area.bottomRight());p.drawLine(area.bottomLeft(),area.topLeft());
    for(int value:{0,25,50,75,100}){int y=area.bottom()-value*area.height()/100;p.setPen(QColor("#414141"));p.drawLine(area.left(),y,area.right(),y);p.setPen(QColor("#aaa"));p.drawText(2,y+4,QString::number(value));}
    const double values[4]={m_rgb,m_ir,m_ambiguous,std::clamp(m_entropy*100.0,0.0,100.0)};const QColor colors[4]={QColor("#3478e5"),QColor("#e76b34"),QColor("#888888"),QColor("#8d63d8")};const QString labels[4]={"RGB","IR","?","Entropy"};int gap=8;int bw=std::max(12,(area.width()-gap*5)/4);
    for(int i=0;i<4;++i){int x=area.left()+gap+(bw+gap)*i;int h=int(area.height()*values[i]/100.0);QRect bar(x,area.bottom()-h,bw,h);p.fillRect(bar,colors[i]);p.setPen(Qt::white);p.drawText(QRect(x,area.bottom()+5,bw,20),Qt::AlignCenter,labels[i]);p.drawText(QRect(x,bar.top()-20,bw,18),Qt::AlignCenter,QString::number(values[i],'f',1));}
    p.setPen(QColor("#ddd"));p.drawText(QRect(0,5,width(),24),Qt::AlignCenter,"Reliability histogram (%)");
}

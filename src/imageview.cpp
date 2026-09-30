#include "imageview.h"
#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>

ImageView::ImageView(QWidget *parent):QGraphicsView(parent){setScene(&m_scene);setBackgroundBrush(QColor("#121619"));setRenderHint(QPainter::Antialiasing);setDragMode(ScrollHandDrag);setMinimumHeight(270);}
void ImageView::setFrame(const QImage &image,const QList<OrientedBox> &boxes){m_scene.clear();m_scene.addPixmap(QPixmap::fromImage(image));for(auto &b:boxes){QPolygonF p({{-b.size.width()/2,-b.size.height()/2},{b.size.width()/2,-b.size.height()/2},{b.size.width()/2,b.size.height()/2},{-b.size.width()/2,b.size.height()/2}});QTransform t;t.translate(b.center.x(),b.center.y());t.rotate(b.angleDegrees);auto color=b.groundTruth?QColor("#63d16e"):QColor("#6ea0ff");auto *item=m_scene.addPolygon(t.map(p),QPen(color,2,Qt::DashLine));item->setToolTip(QString("%1 %2").arg(b.className).arg(b.confidence,0,'f',2));if(!b.groundTruth){auto *label=m_scene.addText(QString("%1 %2").arg(b.className).arg(b.confidence,0,'f',2));label->setDefaultTextColor(color);label->setPos(t.map(p).boundingRect().topLeft());}}setSceneRect(image.rect());fitInView(sceneRect(),Qt::KeepAspectRatio);m_zoom=transform().m11();}
void ImageView::wheelEvent(QWheelEvent *e){double f=e->angleDelta().y()>0?1.15:1.0/1.15;scale(f,f);m_zoom=transform().m11();Q_EMIT zoomChanged(m_zoom);e->accept();}
void ImageView::setSynchronizedZoom(double s){if(s<=0)return;resetTransform();scale(s,s);m_zoom=s;}

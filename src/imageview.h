#pragma once
#include "types.h"
#include <QGraphicsView>

class ImageView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ImageView(QWidget *parent=nullptr);
    void setFrame(const QImage &image,const QList<OrientedBox> &boxes);
Q_SIGNALS:
    void zoomChanged(double scale);
public Q_SLOTS:
    void setSynchronizedZoom(double scale);
protected:
    void wheelEvent(QWheelEvent *event) override;
private:
    QGraphicsScene m_scene;
    double m_zoom=1.0;
};

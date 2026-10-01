#pragma once
#include <QWidget>

class ReliabilityChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ReliabilityChartWidget(QWidget *parent=nullptr);
    void setMetrics(double rgb,double ir,double ambiguous,double entropy);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    double m_rgb=0,m_ir=0,m_ambiguous=0,m_entropy=0;
};

#pragma once
#include "types.h"
#include <QMainWindow>

class ImageView;class BenchmarkWorker;class QThread;class QLineEdit;class QComboBox;class QDoubleSpinBox;class QSpinBox;class QRadioButton;class QCheckBox;class QPushButton;class QProgressBar;class QTableWidget;class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent=nullptr);
    ~MainWindow() override;
private Q_SLOTS:
    void chooseDataset(); void startBenchmark(); void showFrame(FrameResult); void showReport(BenchmarkReport); void showError(QString); void exportReport();
private:
    void setRunning(bool); void fillTable(const BenchmarkReport&);
    QLineEdit *m_dataset;QComboBox *m_task,*m_inputSize;QDoubleSpinBox *m_confidence,*m_nms;QSpinBox *m_batch;QRadioButton *m_single,*m_batchMode;QCheckBox *m_gate;QPushButton *m_run,*m_export;QProgressBar *m_progress;QTableWidget *m_table;ImageView *m_rgb,*m_ir;QLabel *m_rgbName,*m_irName,*m_timing,*m_status;QThread *m_thread;BenchmarkWorker *m_worker;BenchmarkReport m_lastReport;
Q_SIGNALS: void runRequested(BenchmarkConfig); void cancelRequested();
};

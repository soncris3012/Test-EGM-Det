#include "mainwindow.h"
#include "benchmarkworker.h"
#include "imageview.h"
#include "ui_mainwindow.h"
#include <QCoreApplication>
#include <QDir>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextStream>
#include <QThread>

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent){
    qRegisterMetaType<BenchmarkConfig>();qRegisterMetaType<FrameResult>();qRegisterMetaType<BenchmarkReport>();
    Ui::MainWindow ui;ui.setupUi(this);
    m_dataset=ui.datasetEdit;m_task=ui.taskCombo;m_inputSize=ui.inputCombo;
    m_confidence=ui.confidenceSpin;m_nms=ui.nmsSpin;m_batch=ui.batchSpin;
    m_single=ui.singleRadio;m_batchMode=ui.batchRadio;m_gate=ui.gateCheck;
    m_run=ui.runButton;m_export=ui.exportButton;m_progress=ui.progressBar;m_table=ui.metricsTable;
    m_rgb=ui.rgbView;m_ir=ui.irView;m_rgbName=ui.rgbNameLabel;m_irName=ui.irNameLabel;
    m_timing=ui.timingLabel;m_status=ui.statusLabel;
    m_run->setObjectName("run");
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);m_table->verticalHeader()->hide();
    connect(ui.datasetBrowseButton,&QPushButton::clicked,this,&MainWindow::chooseDataset);
    connect(m_run,&QPushButton::clicked,this,&MainWindow::startBenchmark);
    connect(m_export,&QPushButton::clicked,this,&MainWindow::exportReport);
    connect(m_rgb,&ImageView::zoomChanged,m_ir,&ImageView::setSynchronizedZoom);
    connect(m_ir,&ImageView::zoomChanged,m_rgb,&ImageView::setSynchronizedZoom);
    setStyleSheet(R"(
      *{font-family:"Arial",sans-serif;font-size:13px;color:#e2e2e2}
      QMainWindow,QWidget{background:#2b2b2b}
      QGroupBox{background:#303030;border:1px solid #777;margin-top:9px;padding:10px 5px 5px}
      QGroupBox::title{subcontrol-origin:margin;left:7px;padding:0 3px;font-weight:bold}
      QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox{background:#202020;border:1px solid #777;padding:3px;min-height:22px}
      QLineEdit:focus,QComboBox:focus,QSpinBox:focus,QDoubleSpinBox:focus{border:1px solid #aaa}
      QPushButton{background:#444;border:1px outset #999;padding:5px;min-height:24px}
      QPushButton:pressed{border-style:inset;background:#333}QPushButton#run{color:#fff;background:#3b4c62}QPushButton:disabled{color:#777;background:#383838}
      QTableWidget{background:#252525;border:1px solid #777;gridline-color:#555;selection-background-color:#385478}
      QHeaderView::section{background:#444;border:1px solid #666;padding:5px;font-weight:bold}
      QProgressBar{border:1px solid #777;background:#202020;text-align:center;min-height:18px}QProgressBar::chunk{background:#4c76a8}
    )");
    m_thread=new QThread(this);m_worker=new BenchmarkWorker;m_worker->moveToThread(m_thread);connect(m_thread,&QThread::finished,m_worker,&QObject::deleteLater);connect(this,&MainWindow::runRequested,m_worker,&BenchmarkWorker::run);connect(this,&MainWindow::cancelRequested,m_worker,&BenchmarkWorker::cancel,Qt::DirectConnection);connect(m_worker,&BenchmarkWorker::progressChanged,this,[this](int n,int total){m_progress->setMaximum(total);m_progress->setValue(n);m_progress->setFormat(QString("%1 / %2").arg(n).arg(total));});connect(m_worker,&BenchmarkWorker::frameProcessed,this,&MainWindow::showFrame);connect(m_worker,&BenchmarkWorker::benchmarkFinished,this,&MainWindow::showReport);connect(m_worker,&BenchmarkWorker::failed,this,&MainWindow::showError);m_thread->start();
}
MainWindow::~MainWindow(){Q_EMIT cancelRequested();m_thread->quit();m_thread->wait();}

void MainWindow::chooseDataset(){auto p=QFileDialog::getExistingDirectory(this,"Choose paired RGB/IR dataset");if(!p.isEmpty())m_dataset->setText(p);}
void MainWindow::startBenchmark(){if(m_dataset->text().isEmpty()){QMessageBox::information(this,"Dataset required","Choose a dataset root containing paired RGB and IR folders.");return;}QString model=QDir(QCoreApplication::applicationDirPath()).filePath("models/egm_det.onnx");if(!QFileInfo::exists(model))model=QDir(QCoreApplication::applicationDirPath()).filePath("../models/egm_det.onnx");if(!QFileInfo::exists(model))model=QDir::current().filePath("models/egm_det.onnx");BenchmarkConfig c{m_dataset->text(),m_task->currentText(),model,m_confidence->value(),m_nms->value(),m_batch->value(),m_inputSize->currentText().toInt(),m_single->isChecked(),m_gate->isChecked()};setRunning(true);m_status->setText(QFileInfo::exists(model)?"Model found automatically: models/egm_det.onnx":"Model not installed; running the deterministic demo evaluator.");Q_EMIT runRequested(c);}
void MainWindow::setRunning(bool b){m_run->setEnabled(!b);m_export->setEnabled(!b&&!m_lastReport.rows.isEmpty());if(b){m_progress->setRange(0,0);m_table->setRowCount(0);}}
void MainWindow::showFrame(FrameResult f){m_rgbName->setText(f.fileName);m_irName->setText(f.fileName);m_rgb->setFrame(f.rgb,f.boxes);m_ir->setFrame(f.ir,f.boxes);double total=f.preprocessMs+f.forwardMs+f.nmsMs;m_timing->setText(QString("Pre %1 ms · Forward %2 ms · NMS %3 ms · %4 FPS").arg(f.preprocessMs,0,'f',1).arg(f.forwardMs,0,'f',1).arg(f.nmsMs,0,'f',1).arg(total>0?1000/total:0,0,'f',0));}
void MainWindow::showReport(BenchmarkReport r){m_lastReport=r;fillTable(r);m_progress->setRange(0,qMax(1,r.total));m_progress->setValue(r.processed);m_status->setText(QString("Completed %1/%2 pairs in %3 s").arg(r.processed).arg(r.total).arg(r.elapsedMs/1000,0,'f',2));setRunning(false);}
void MainWindow::showError(QString s){setRunning(false);m_progress->setRange(0,1);m_status->setText("Error: "+s);QMessageBox::warning(this,"Benchmark error",s);}
void MainWindow::fillTable(const BenchmarkReport&r){m_table->setRowCount(r.rows.size());for(int i=0;i<r.rows.size();++i){auto&m=r.rows[i];QStringList vals{m.className,QString::number(m.targets),QString::number(m.detections),QString::number(m.precision,'f',1),QString::number(m.recall,'f',1),QString::number(m.ap50,'f',1),QString::number(m.ap75,'f',1),QString::number(m.map5095,'f',1)};for(int c=0;c<vals.size();++c){auto*item=new QTableWidgetItem(vals[c]);if(i==r.rows.size()-1){item->setBackground(QColor("#123568"));item->setForeground(QColor("#8cb7ff"));}m_table->setItem(i,c,item);}}}
void MainWindow::exportReport(){QString path=QFileDialog::getSaveFileName(this,"Export report","egm-det-report.csv","CSV (*.csv);;PDF (*.pdf)");if(path.isEmpty())return;if(path.endsWith(".pdf",Qt::CaseInsensitive)){QPdfWriter pdf(path);pdf.setPageSize(QPageSize::A4);QPainter p(&pdf);p.setFont(QFont("Helvetica",18,QFont::Bold));p.drawText(500,600,"EGM-Det Benchmark Report");p.setFont(QFont("Helvetica",10));int y=1100;p.drawText(500,y,"Class        Target   Det.      P       R      AP50    AP75   AP50-95");for(auto&m:m_lastReport.rows){y+=300;p.drawText(500,y,QString("%1   %2   %3   %4   %5   %6   %7   %8").arg(m.className,-14).arg(m.targets).arg(m.detections).arg(m.precision,0,'f',1).arg(m.recall,0,'f',1).arg(m.ap50,0,'f',1).arg(m.ap75,0,'f',1).arg(m.map5095,0,'f',1));}p.end();}else{if(!path.endsWith(".csv"))path+=".csv";QFile f(path);if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){showError(f.errorString());return;}QTextStream o(&f);o<<"Class,Target,Detected,Precision,Recall,AP50,AP75,AP50-95\n";for(auto&m:m_lastReport.rows)o<<m.className<<','<<m.targets<<','<<m.detections<<','<<m.precision<<','<<m.recall<<','<<m.ap50<<','<<m.ap75<<','<<m.map5095<<'\n';}m_status->setText("Exported report to "+path);}

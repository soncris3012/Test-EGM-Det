#include "mainwindow.h"
#include "benchmarkworker.h"
#include "imageview.h"
#include "reliabilitychartwidget.h"
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
    m_rgb=ui.rgbView;m_ir=ui.irView;m_chart=ui.reliabilityChart;m_rgbName=ui.rgbNameLabel;m_irName=ui.irNameLabel;
    m_timing=ui.timingLabel;m_status=ui.statusLabel;
    ui.imagesLayout->setStretch(0,1);ui.imagesLayout->setStretch(1,1);
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
void MainWindow::startBenchmark(){if(m_dataset->text().isEmpty()){QMessageBox::information(this,"Dataset required","Choose a dataset root containing paired RGB and IR folders.");return;}QString model=QDir(QCoreApplication::applicationDirPath()).filePath("models/egm_det.onnx");if(!QFileInfo::exists(model))model=QDir(QCoreApplication::applicationDirPath()).filePath("../models/egm_det.onnx");if(!QFileInfo::exists(model))model=QDir::current().filePath("models/egm_det.onnx");m_showReliability=m_gate->isChecked();BenchmarkConfig c{m_dataset->text(),m_task->currentText(),model,m_confidence->value(),m_nms->value(),m_batch->value(),m_inputSize->currentText().toInt(),m_single->isChecked(),m_showReliability};setRunning(true);m_status->setText("Running weight-free RGB/IR reliability analysis (not EGM-Det inference)…");Q_EMIT runRequested(c);}
void MainWindow::setRunning(bool b){m_run->setEnabled(!b);m_export->setEnabled(!b&&(!m_lastReport.rows.isEmpty()||m_lastReport.reliabilityOnly));if(b){m_progress->setRange(0,0);m_table->setRowCount(0);}}
void MainWindow::showFrame(FrameResult f){m_rgbName->setText("RGB | "+f.fileName);m_irName->setText((m_showReliability?"Reliability map | ":"IR | ")+f.fileName);m_rgb->setFrame(f.rgb,f.boxes);m_ir->setFrame(m_showReliability?f.gateMap:f.ir,m_showReliability?QList<OrientedBox>{}:f.boxes);m_chart->setMetrics(f.rgbPreferencePct,f.irPreferencePct,f.ambiguousPct,f.meanEntropy);double total=f.preprocessMs+f.forwardMs;m_timing->setText(QString("RGB %1% | IR %2% | Ambiguous %3% | Entropy %4 | %5 ms").arg(f.rgbPreferencePct,0,'f',1).arg(f.irPreferencePct,0,'f',1).arg(f.ambiguousPct,0,'f',1).arg(f.meanEntropy,0,'f',2).arg(total,0,'f',1));}
void MainWindow::showReport(BenchmarkReport r){m_lastReport=r;fillTable(r);m_progress->setRange(0,qMax(1,r.total));m_progress->setValue(r.processed);m_status->setText(QString("Reliability analysis completed: %1 pairs | RGB %2% | IR %3% | ambiguous %4% (not EGM-Det mAP)").arg(r.processed).arg(r.rgbPreferencePct,0,'f',1).arg(r.irPreferencePct,0,'f',1).arg(r.ambiguousPct,0,'f',1));setRunning(false);}
void MainWindow::showError(QString s){setRunning(false);m_progress->setRange(0,1);m_status->setText("Error: "+s);QMessageBox::warning(this,"Benchmark error",s);}
void MainWindow::fillTable(const BenchmarkReport&r){if(r.reliabilityOnly){m_table->setHorizontalHeaderLabels({"Analysis","RGB prefer","IR prefer","Ambiguous","Mean entropy","Pairs","Mode","mAP"});m_table->setRowCount(1);QStringList vals{"Dataset average",QString::number(r.rgbPreferencePct,'f',1)+"%",QString::number(r.irPreferencePct,'f',1)+"%",QString::number(r.ambiguousPct,'f',1)+"%",QString::number(r.meanEntropy,'f',2),QString::number(r.processed),"Heuristic","N/A"};for(int c=0;c<vals.size();++c)m_table->setItem(0,c,new QTableWidgetItem(vals[c]));return;}m_table->setHorizontalHeaderLabels({"Class","Target","Det.","P","R","AP50","AP75","AP50–95"});m_table->setRowCount(r.rows.size());for(int i=0;i<r.rows.size();++i){auto&m=r.rows[i];QStringList vals{m.className,QString::number(m.targets),QString::number(m.detections),QString::number(m.precision,'f',1),QString::number(m.recall,'f',1),QString::number(m.ap50,'f',1),QString::number(m.ap75,'f',1),QString::number(m.map5095,'f',1)};for(int c=0;c<vals.size();++c)m_table->setItem(i,c,new QTableWidgetItem(vals[c]));}}
void MainWindow::exportReport(){QString path=QFileDialog::getSaveFileName(this,"Export report","rgb-ir-reliability.csv","CSV (*.csv);;PDF (*.pdf)");if(path.isEmpty())return;if(path.endsWith(".pdf",Qt::CaseInsensitive)){QPdfWriter pdf(path);pdf.setPageSize(QPageSize::A4);QPainter p(&pdf);p.setFont(QFont("Helvetica",18,QFont::Bold));p.drawText(500,600,m_lastReport.reliabilityOnly?"RGB-IR Reliability Analysis":"EGM-Det Benchmark Report");p.setFont(QFont("Helvetica",10));int y=1100;if(m_lastReport.reliabilityOnly){p.drawText(500,y,QString("Pairs: %1").arg(m_lastReport.processed));p.drawText(500,y+300,QString("RGB preferred: %1%").arg(m_lastReport.rgbPreferencePct,0,'f',1));p.drawText(500,y+600,QString("IR preferred: %1%").arg(m_lastReport.irPreferencePct,0,'f',1));p.drawText(500,y+900,QString("Ambiguous: %1%").arg(m_lastReport.ambiguousPct,0,'f',1));p.drawText(500,y+1200,QString("Mean normalized entropy: %1").arg(m_lastReport.meanEntropy,0,'f',3));p.drawText(500,y+1700,"Heuristic analysis - not EGM-Det inference or mAP.");}else{p.drawText(500,y,"Class        Target   Det.      P       R      AP50    AP75   AP50-95");for(auto&m:m_lastReport.rows){y+=300;p.drawText(500,y,QString("%1   %2   %3   %4   %5   %6   %7   %8").arg(m.className,-14).arg(m.targets).arg(m.detections).arg(m.precision,0,'f',1).arg(m.recall,0,'f',1).arg(m.ap50,0,'f',1).arg(m.ap75,0,'f',1).arg(m.map5095,0,'f',1));}}p.end();}else{if(!path.endsWith(".csv"))path+=".csv";QFile f(path);if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){showError(f.errorString());return;}QTextStream o(&f);if(m_lastReport.reliabilityOnly){o<<"Pairs,RGBPreferredPct,IRPreferredPct,AmbiguousPct,MeanNormalizedEntropy,Method\n"<<m_lastReport.processed<<','<<m_lastReport.rgbPreferencePct<<','<<m_lastReport.irPreferencePct<<','<<m_lastReport.ambiguousPct<<','<<m_lastReport.meanEntropy<<",Heuristic-not-EGM-Det\n";}else{o<<"Class,Target,Detected,Precision,Recall,AP50,AP75,AP50-95\n";for(auto&m:m_lastReport.rows)o<<m.className<<','<<m.targets<<','<<m.detections<<','<<m.precision<<','<<m.recall<<','<<m.ap50<<','<<m.ap75<<','<<m.map5095<<'\n';}}m_status->setText("Exported report to "+path);}

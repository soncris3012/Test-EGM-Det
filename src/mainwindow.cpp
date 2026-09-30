#include "mainwindow.h"
#include "benchmarkworker.h"
#include "imageview.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
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
#include <QVBoxLayout>

static QPushButton *browseButton(){auto*b=new QPushButton("Browse…");b->setObjectName("secondary");return b;}

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent){
    qRegisterMetaType<BenchmarkConfig>();qRegisterMetaType<FrameResult>();qRegisterMetaType<BenchmarkReport>();
    setWindowTitle("EGM-Det benchmark tool");resize(1480,900);
    auto *root=new QWidget;auto *layout=new QHBoxLayout(root);layout->setContentsMargins(24,18,24,24);layout->setSpacing(20);layout->addWidget(makeConfigPanel(),0);layout->addWidget(makeResultsPanel(),1);setCentralWidget(root);
    setStyleSheet(R"(
      *{font-family:"Inter","SF Pro Display",sans-serif;font-size:15px;color:#deded9} QMainWindow,QWidget{background:#151616}
      QGroupBox{border:1px solid #444746;border-radius:14px;margin-top:13px;padding:18px 14px 14px}QGroupBox::title{subcontrol-origin:margin;left:14px;padding:0 7px;font-size:18px;font-weight:600}
      QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox{background:#1b1c1c;border:1px solid #3b3d3d;border-radius:10px;padding:9px;min-height:24px}QLineEdit:focus,QComboBox:focus,QSpinBox:focus,QDoubleSpinBox:focus{border-color:#4c83df}
      QPushButton{background:#1a1b1b;border:1px solid #3f4241;border-radius:10px;padding:11px;font-size:17px}QPushButton:hover{background:#242626}QPushButton#run{color:#86b4ff;border-color:#245aa4}QPushButton:disabled{color:#777}
      QTableWidget{background:#171818;border:0;gridline-color:#333636;selection-background-color:#123568}QHeaderView::section{background:#1a1b1b;border:0;border-bottom:1px solid #3c3e3e;padding:10px;font-weight:600}QProgressBar{border:1px solid #3a3d3c;border-radius:8px;background:#111212;text-align:center}QProgressBar::chunk{background:#477fd3;border-radius:7px}
    )");
    m_thread=new QThread(this);m_worker=new BenchmarkWorker;m_worker->moveToThread(m_thread);connect(m_thread,&QThread::finished,m_worker,&QObject::deleteLater);connect(this,&MainWindow::runRequested,m_worker,&BenchmarkWorker::run);connect(this,&MainWindow::cancelRequested,m_worker,&BenchmarkWorker::cancel,Qt::DirectConnection);connect(m_worker,&BenchmarkWorker::progressChanged,this,[this](int n,int total){m_progress->setMaximum(total);m_progress->setValue(n);m_progress->setFormat(QString("%1 / %2").arg(n).arg(total));});connect(m_worker,&BenchmarkWorker::frameProcessed,this,&MainWindow::showFrame);connect(m_worker,&BenchmarkWorker::benchmarkFinished,this,&MainWindow::showReport);connect(m_worker,&BenchmarkWorker::failed,this,&MainWindow::showError);m_thread->start();
}
MainWindow::~MainWindow(){Q_EMIT cancelRequested();m_thread->quit();m_thread->wait();}

QWidget *MainWindow::makeConfigPanel(){
    auto *panel=new QWidget;panel->setFixedWidth(380);auto *v=new QVBoxLayout(panel);v->setContentsMargins(0,0,0,0);
    auto *config=new QGroupBox("Test configuration");auto *cv=new QVBoxLayout(config);auto *dataRow=new QHBoxLayout;m_dataset=new QLineEdit;m_dataset->setPlaceholderText("Dataset root folder");auto *db=browseButton();dataRow->addWidget(m_dataset);dataRow->addWidget(db);cv->addLayout(dataRow);auto *modelRow=new QHBoxLayout;m_model=new QLineEdit;m_model->setPlaceholderText("Optional .onnx model");auto *mb=browseButton();modelRow->addWidget(m_model);modelRow->addWidget(mb);cv->addLayout(modelRow);auto *form=new QFormLayout;m_task=new QComboBox;m_task->addItems({"DroneVehicle — OBB","VEDAI — OBB","LLVIP — HBB"});form->addRow("Task",m_task);cv->addLayout(form);v->addWidget(config);
    connect(db,&QPushButton::clicked,this,&MainWindow::chooseDataset);connect(mb,&QPushButton::clicked,this,&MainWindow::chooseModel);
    auto *params=new QGroupBox("Parameters");auto *f=new QFormLayout(params);m_confidence=new QDoubleSpinBox;m_confidence->setRange(0,1);m_confidence->setSingleStep(.05);m_confidence->setValue(.25);m_nms=new QDoubleSpinBox;m_nms->setRange(0,1);m_nms->setSingleStep(.05);m_nms->setValue(.45);m_batch=new QSpinBox;m_batch->setRange(1,128);m_inputSize=new QComboBox;m_inputSize->addItems({"640","1024"});f->addRow("Confidence",m_confidence);f->addRow("NMS IoU",m_nms);f->addRow("Batch size",m_batch);f->addRow("Input size",m_inputSize);v->addWidget(params);
    auto *mode=new QGroupBox("Mode");auto *mv=new QVBoxLayout(mode);m_single=new QRadioButton("Single pair debug");m_batchMode=new QRadioButton("Batch evaluation");m_batchMode->setChecked(true);m_gate=new QCheckBox("Show Modality Gate");mv->addWidget(m_single);mv->addWidget(m_batchMode);mv->addWidget(m_gate);v->addWidget(mode);
    m_run=new QPushButton("▷  Run benchmark");m_run->setObjectName("run");m_export=new QPushButton("⇩  Export CSV / PDF");m_export->setEnabled(false);v->addWidget(m_run);v->addWidget(m_export);m_status=new QLabel("Ready — select a dataset folder");m_status->setWordWrap(true);m_status->setStyleSheet("color:#8f9290;padding:6px");v->addWidget(m_status);v->addStretch();connect(m_run,&QPushButton::clicked,this,&MainWindow::startBenchmark);connect(m_export,&QPushButton::clicked,this,&MainWindow::exportReport);return panel;
}

QWidget *MainWindow::makeResultsPanel(){
    auto *panel=new QWidget;auto *v=new QVBoxLayout(panel);v->setContentsMargins(0,0,0,0);auto *images=new QHBoxLayout;
    auto makeView=[&](QString title,ImageView **view,QLabel **name){auto*g=new QGroupBox(title);auto*l=new QVBoxLayout(g);*name=new QLabel("No image");(*name)->setAlignment(Qt::AlignRight);*view=new ImageView;l->addWidget(*name);l->addWidget(*view);images->addWidget(g);};makeView("RGB image",&m_rgb,&m_rgbName);makeView("IR image",&m_ir,&m_irName);v->addLayout(images,5);connect(m_rgb,&ImageView::zoomChanged,m_ir,&ImageView::setSynchronizedZoom);connect(m_ir,&ImageView::zoomChanged,m_rgb,&ImageView::setSynchronizedZoom);
    auto *summary=new QHBoxLayout;auto *legend=new QLabel("<font color='#63d16e'>- - Ground truth</font>   <font color='#6ea0ff'>━━ Prediction</font>");m_timing=new QLabel("Pre — · Forward — · NMS — · FPS —");m_timing->setAlignment(Qt::AlignRight);summary->addWidget(legend);summary->addStretch();summary->addWidget(m_timing);v->addLayout(summary);m_progress=new QProgressBar;m_progress->setRange(0,1);m_progress->setValue(0);v->addWidget(m_progress);
    m_table=new QTableWidget(0,8);m_table->setHorizontalHeaderLabels({"Class","Target","Det.","P","R","AP50","AP75","AP50–95"});m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);m_table->verticalHeader()->hide();m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);m_table->setSelectionBehavior(QAbstractItemView::SelectRows);v->addWidget(m_table,4);auto *note=new QLabel("Metrics use rotated IoU and COCO-style 101-point AP at IoU 0.50:0.05:0.95.");note->setStyleSheet("color:#8c8e8c");v->addWidget(note);return panel;
}

void MainWindow::chooseDataset(){auto p=QFileDialog::getExistingDirectory(this,"Choose paired RGB/IR dataset");if(!p.isEmpty())m_dataset->setText(p);}
void MainWindow::chooseModel(){auto p=QFileDialog::getOpenFileName(this,"Choose EGM-Det ONNX model",{},"ONNX model (*.onnx)");if(!p.isEmpty())m_model->setText(p);}
void MainWindow::startBenchmark(){if(m_dataset->text().isEmpty()){QMessageBox::information(this,"Dataset required","Choose a dataset root containing paired RGB and IR folders.");return;}BenchmarkConfig c{m_dataset->text(),m_task->currentText(),m_model->text(),m_confidence->value(),m_nms->value(),m_batch->value(),m_inputSize->currentText().toInt(),m_single->isChecked(),m_gate->isChecked()};setRunning(true);m_status->setText(m_model->text().isEmpty()?"Running evaluator with the deterministic demo backend…":"ONNX path saved; this portable build uses the demo backend until ONNX Runtime is enabled.");Q_EMIT runRequested(c);}
void MainWindow::setRunning(bool b){m_run->setEnabled(!b);m_export->setEnabled(!b&&!m_lastReport.rows.isEmpty());if(b){m_progress->setRange(0,0);m_table->setRowCount(0);}}
void MainWindow::showFrame(FrameResult f){m_rgbName->setText(f.fileName);m_irName->setText(f.fileName);m_rgb->setFrame(f.rgb,f.boxes);m_ir->setFrame(f.ir,f.boxes);double total=f.preprocessMs+f.forwardMs+f.nmsMs;m_timing->setText(QString("Pre %1 ms · Forward %2 ms · NMS %3 ms · %4 FPS").arg(f.preprocessMs,0,'f',1).arg(f.forwardMs,0,'f',1).arg(f.nmsMs,0,'f',1).arg(total>0?1000/total:0,0,'f',0));}
void MainWindow::showReport(BenchmarkReport r){m_lastReport=r;fillTable(r);m_progress->setRange(0,qMax(1,r.total));m_progress->setValue(r.processed);m_status->setText(QString("Completed %1/%2 pairs in %3 s").arg(r.processed).arg(r.total).arg(r.elapsedMs/1000,0,'f',2));setRunning(false);}
void MainWindow::showError(QString s){setRunning(false);m_progress->setRange(0,1);m_status->setText("Error: "+s);QMessageBox::warning(this,"Benchmark error",s);}
void MainWindow::fillTable(const BenchmarkReport&r){m_table->setRowCount(r.rows.size());for(int i=0;i<r.rows.size();++i){auto&m=r.rows[i];QStringList vals{m.className,QString::number(m.targets),QString::number(m.detections),QString::number(m.precision,'f',1),QString::number(m.recall,'f',1),QString::number(m.ap50,'f',1),QString::number(m.ap75,'f',1),QString::number(m.map5095,'f',1)};for(int c=0;c<vals.size();++c){auto*item=new QTableWidgetItem(vals[c]);if(i==r.rows.size()-1){item->setBackground(QColor("#123568"));item->setForeground(QColor("#8cb7ff"));}m_table->setItem(i,c,item);}}}
void MainWindow::exportReport(){QString path=QFileDialog::getSaveFileName(this,"Export report","egm-det-report.csv","CSV (*.csv);;PDF (*.pdf)");if(path.isEmpty())return;if(path.endsWith(".pdf",Qt::CaseInsensitive)){QPdfWriter pdf(path);pdf.setPageSize(QPageSize::A4);QPainter p(&pdf);p.setFont(QFont("Helvetica",18,QFont::Bold));p.drawText(500,600,"EGM-Det Benchmark Report");p.setFont(QFont("Helvetica",10));int y=1100;p.drawText(500,y,"Class        Target   Det.      P       R      AP50    AP75   AP50-95");for(auto&m:m_lastReport.rows){y+=300;p.drawText(500,y,QString("%1   %2   %3   %4   %5   %6   %7   %8").arg(m.className,-14).arg(m.targets).arg(m.detections).arg(m.precision,0,'f',1).arg(m.recall,0,'f',1).arg(m.ap50,0,'f',1).arg(m.ap75,0,'f',1).arg(m.map5095,0,'f',1));}p.end();}else{if(!path.endsWith(".csv"))path+=".csv";QFile f(path);if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){showError(f.errorString());return;}QTextStream o(&f);o<<"Class,Target,Detected,Precision,Recall,AP50,AP75,AP50-95\n";for(auto&m:m_lastReport.rows)o<<m.className<<','<<m.targets<<','<<m.detections<<','<<m.precision<<','<<m.recall<<','<<m.ap50<<','<<m.ap75<<','<<m.map5095<<'\n';}m_status->setText("Exported report to "+path);}

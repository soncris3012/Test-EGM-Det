#include "dataloader.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

DataLoader::DataLoader(QString root):m_root(std::move(root)){}

static QString firstDirectory(const QString &root,const QStringList &names){for(auto &n:names){QString p=QDir(root).filePath(n);if(QDir(p).exists())return p;}return {};}

static QString datasetRoot(QString selected){
    QDir dir(selected);QString leaf=dir.dirName().toLower();
    if(leaf=="train"||leaf=="test"||leaf=="val"||leaf=="validation"){dir.cdUp();leaf=dir.dirName().toLower();}
    if(leaf=="visible"||leaf=="infrared"||leaf=="rgb"||leaf=="ir"||leaf=="crop_hr_visible"||leaf=="crop_lr_visible"||leaf=="cropinfrared")dir.cdUp();
    return dir.absolutePath();
}

static QString withCommonSplit(const QString &rgb,const QString &ir,QString *resolvedIr){
    for(const auto &split:{"test","val","validation","train"}){
        const QString r=QDir(rgb).filePath(split),i=QDir(ir).filePath(split);
        if(QDir(r).exists()&&QDir(i).exists()){*resolvedIr=i;return r;}
    }
    *resolvedIr=ir;return rgb;
}

QList<Sample> DataLoader::discover(QString *error) const {
    const QString root=datasetRoot(m_root);
    QString rgb=firstDirectory(root,{"rgb","RGB","images/rgb","visible","Visible","crop_HR_visible","crop_LR_visible","images"});
    QString ir=firstDirectory(root,{"ir","IR","images/ir","infrared","Infrared","cropinfrared"});
    QString ann=firstDirectory(root,{"labels","annotations","gt","ground_truth"});
    if(rgb.isEmpty()||ir.isEmpty()){if(error)*error="Expected paired folders: rgb/ir, visible/infrared, images/rgb/images/ir, or RoadScene crop_HR_visible/cropinfrared.";return {};}
    QString resolvedIr;rgb=withCommonSplit(rgb,ir,&resolvedIr);ir=resolvedIr;
    const QString split=QFileInfo(rgb).fileName().toLower();if(!ann.isEmpty()&&QDir(QDir(ann).filePath(split)).exists())ann=QDir(ann).filePath(split);
    QDir d(rgb); const QStringList files=d.entryList({"*.jpg","*.jpeg","*.png","*.bmp"},QDir::Files,QDir::Name); QList<Sample> out;
    for(auto &f:files){QFileInfo fi(f);QString irp;for(auto&e:{"jpg","jpeg","png","bmp"}){auto p=QDir(ir).filePath(fi.completeBaseName()+"."+e);if(QFile::exists(p)){irp=p;break;}}if(irp.isEmpty())continue;Sample s{fi.completeBaseName(),d.filePath(f),irp,{}};if(!ann.isEmpty()){for(auto&e:{"txt","csv"}){auto p=QDir(ann).filePath(s.id+"."+e);if(QFile::exists(p)){s.annotationPath=p;break;}}}out<<s;}
    if(out.isEmpty()&&error)*error=QString("No paired RGB/IR images with matching base names were found in %1 and %2.").arg(rgb,ir); return out;
}

QList<OrientedBox> DataLoader::loadAnnotation(const QString &path,const QSize &size,const QString &id){
    QList<OrientedBox> out;QFile f(path);if(!f.open(QIODevice::ReadOnly|QIODevice::Text))return out;QTextStream in(&f);
    while(!in.atEnd()){auto line=in.readLine().trimmed();if(line.isEmpty()||line.startsWith('#'))continue;auto p=line.split(QRegularExpression("[,\\s]+"),Qt::SkipEmptyParts);if(p.size()<6)continue;bool ok=true;QString cls=p[0];double cx=p[1].toDouble(&ok);if(!ok)continue;double cy=p[2].toDouble(),w=p[3].toDouble(),h=p[4].toDouble(),a=p[5].toDouble();if(cx<=1&&cy<=1&&w<=1&&h<=1){cx*=size.width();cy*=size.height();w*=size.width();h*=size.height();}out<<OrientedBox{cls,{cx,cy},{w,h},a,1.0,true,id};
    }return out;
}

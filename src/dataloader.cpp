#include "dataloader.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

DataLoader::DataLoader(QString root):m_root(std::move(root)){}

static QString firstDirectory(const QString &root,const QStringList &names){for(auto &n:names){QString p=QDir(root).filePath(n);if(QDir(p).exists())return p;}return {};}

QList<Sample> DataLoader::discover(QString *error) const {
    const QString rgb=firstDirectory(m_root,{"rgb","RGB","images/rgb","visible","images"});
    const QString ir=firstDirectory(m_root,{"ir","IR","images/ir","infrared"});
    const QString ann=firstDirectory(m_root,{"labels","annotations","gt","ground_truth"});
    if(rgb.isEmpty()||ir.isEmpty()){if(error)*error="Expected RGB and IR folders (rgb/ir, visible/infrared, or images/rgb/images/ir).";return {};}
    QDir d(rgb); const QStringList files=d.entryList({"*.jpg","*.jpeg","*.png","*.bmp"},QDir::Files,QDir::Name); QList<Sample> out;
    for(auto &f:files){QFileInfo fi(f);QString irp;for(auto&e:{"jpg","jpeg","png","bmp"}){auto p=QDir(ir).filePath(fi.completeBaseName()+"."+e);if(QFile::exists(p)){irp=p;break;}}if(irp.isEmpty())continue;Sample s{fi.completeBaseName(),d.filePath(f),irp,{}};if(!ann.isEmpty()){for(auto&e:{"txt","csv"}){auto p=QDir(ann).filePath(s.id+"."+e);if(QFile::exists(p)){s.annotationPath=p;break;}}}out<<s;}
    if(out.isEmpty()&&error)*error="No paired RGB/IR images with matching base names were found."; return out;
}

QList<OrientedBox> DataLoader::loadAnnotation(const QString &path,const QSize &size,const QString &id){
    QList<OrientedBox> out;QFile f(path);if(!f.open(QIODevice::ReadOnly|QIODevice::Text))return out;QTextStream in(&f);
    while(!in.atEnd()){auto line=in.readLine().trimmed();if(line.isEmpty()||line.startsWith('#'))continue;auto p=line.split(QRegularExpression("[,\\s]+"),Qt::SkipEmptyParts);if(p.size()<6)continue;bool ok=true;QString cls=p[0];double cx=p[1].toDouble(&ok);if(!ok)continue;double cy=p[2].toDouble(),w=p[3].toDouble(),h=p[4].toDouble(),a=p[5].toDouble();if(cx<=1&&cy<=1&&w<=1&&h<=1){cx*=size.width();cy*=size.height();w*=size.width();h*=size.height();}out<<OrientedBox{cls,{cx,cy},{w,h},a,1.0,true,id};
    }return out;
}

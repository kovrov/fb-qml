#include <QDir>
#include <QSettings>
#include "qdebug.h"

#include "datafs.h"



DataFS *DataFS::self = NULL;


DataFS *DataFS::instance()
{
    if (NULL == DataFS::self)
        DataFS::self = new DataFS;
    return DataFS::self;
}


DataFS::DataFS()
{
    QDir datapath(QSettings().value("datapath").toString());
    foreach (const QFileInfo &fi, datapath.entryInfoList(QDir::Files)) {
        m_files[fi.fileName()] = fi;
    }
}

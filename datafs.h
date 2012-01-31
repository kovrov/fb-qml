#ifndef DATAFS_H
#define DATAFS_H

#include <QMap>
#include <QString>
#include <QFileInfo>



class DataFS
{
public:
    static DataFS *instance();
    static QFileInfo fileInfo(const QString &name) { return instance()->m_files[name]; }

private:
    DataFS();
    static DataFS *self;

    QMap<QString, QFileInfo> m_files;
};



#endif // DATAFS_H

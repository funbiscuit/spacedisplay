#ifndef SPACEDISPLAY_FILEENTRYPOPUP_H
#define SPACEDISPLAY_FILEENTRYPOPUP_H

#include <QObject>
#include <QAction>
#include <memory>
#include <functional>
#include <string>

#include "filepath.h"

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

class SpaceScanner;

class FileEntryPopup : public QObject
{
Q_OBJECT

public:
    FileEntryPopup(QWidget* _parent);

    void popupDir(std::unique_ptr<FilePath> dir_path);
    void popupFile(std::unique_ptr<FilePath> file_path);
    void updateActions(SpaceScanner* scanner);

    std::function<void(const FilePath&)> onRescanListener = nullptr;

private slots:
    void onRescan();
    void onShow();
    void onOpen();
    void onDeleteDir();
    void onDeleteFile();
    void onProperties();


protected:
    std::unique_ptr<FilePath> currentEntryPath;
    std::string currentEntryName;

    QWidget* parent;

    //entry popup actions
    std::unique_ptr<QAction> rescanAct;
    std::unique_ptr<QAction> openInFMAct;
    std::unique_ptr<QAction> showInFMAct;
    std::unique_ptr<QAction> deleteDirAct;
    std::unique_ptr<QAction> deleteFileAct;
    std::unique_ptr<QAction> propertiesAct;
};


#endif //SPACEDISPLAY_FILEENTRYPOPUP_H

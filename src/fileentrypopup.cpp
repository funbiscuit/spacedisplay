#include <utility>

#include <QtWidgets>

#include <iostream>
#include "fileentrypopup.h"
#include "spacescanner.h"
#include "utils.h"

FileEntryPopup::FileEntryPopup(QWidget* _parent) : parent(_parent)
{

    rescanAct = std::make_unique<QAction>("Rescan folder", parent);
    rescanAct->setEnabled(false);
    connect(rescanAct.get(), &QAction::triggered, this, &FileEntryPopup::onRescan);

    openInFMAct = std::make_unique<QAction>("Open in File Manager", parent);
    connect(openInFMAct.get(), &QAction::triggered, this, &FileEntryPopup::onOpen);

    showInFMAct = std::make_unique<QAction>("Show in File Manager", parent);
    connect(showInFMAct.get(), &QAction::triggered, this, &FileEntryPopup::onShow);

    deleteDirAct = std::make_unique<QAction>("Delete (recursively)", parent);
    deleteDirAct->setEnabled(false);
    connect(deleteDirAct.get(), &QAction::triggered, this, &FileEntryPopup::onDeleteDir);

    deleteFileAct = std::make_unique<QAction>("Delete", parent);
    deleteFileAct->setEnabled(false);
    connect(deleteFileAct.get(), &QAction::triggered, this, &FileEntryPopup::onDeleteFile);

    propertiesAct = std::make_unique<QAction>("Properties", parent);
    propertiesAct->setEnabled(false);
    connect(propertiesAct.get(), &QAction::triggered, this, &FileEntryPopup::onProperties);

}

void FileEntryPopup::updateActions(SpaceScanner* scanner)
{
    if(scanner)
    {
//        rescanAct->setEnabled(scanner->can_refresh());
        //todo maybe allow to delete while scanning
//        deleteDirAct->setEnabled(scanner->can_refresh());
    }
}

void FileEntryPopup::onRescan()
{
    std::cout<<"rescan\n";
}
void FileEntryPopup::onDeleteDir()
{
    std::cout<<"delete dir\n";
}
void FileEntryPopup::onDeleteFile()
{
    std::cout<<"delete file\n";
}
void FileEntryPopup::onProperties()
{
    std::cout<<"properties\n";
}

void FileEntryPopup::onShow()
{
    show_file_in_file_manager(currentEntryPath.c_str());
}

void FileEntryPopup::onOpen()
{
    open_folder_in_file_manager(currentEntryPath.c_str());
}

void FileEntryPopup::popupDir(std::string dir_path)
{
    currentEntryPath = std::move(dir_path);
    parseName();

    QMenu menu(parent);
    auto title = menu.addAction(currentEntryName.c_str());
    title->setEnabled(false);
    menu.addSeparator();
    menu.addAction(rescanAct.get());
    menu.addAction(openInFMAct.get());
    menu.addAction(deleteDirAct.get());
    menu.addAction(propertiesAct.get());

    menu.exec(QCursor::pos()+QPoint(10,10));
}

void FileEntryPopup::popupFile(std::string file_path)
{
    currentEntryPath = std::move(file_path);
    parseName();

    QMenu menu(parent);

    auto title = menu.addAction(currentEntryName.c_str());
    title->setEnabled(false);
    menu.addSeparator();
    menu.addAction(showInFMAct.get());
    menu.addAction(deleteFileAct.get());
    menu.addAction(propertiesAct.get());

    menu.exec(QCursor::pos()+QPoint(10,10));
}

void FileEntryPopup::parseName()
{
    bool skipLast = false;
    auto pos=currentEntryPath.find_last_of('/');
    if(pos==currentEntryPath.length()-1 && pos>0)
    {
        pos=currentEntryPath.find_last_of('/',currentEntryPath.length()-2);
        skipLast = true;
    }
    if(pos!=std::string::npos && (skipLast || pos!=0))
        currentEntryName=currentEntryPath.substr(pos+1,
                skipLast ? currentEntryPath.length()-pos-2 : std::string::npos);
    else
        currentEntryName=currentEntryPath;
}

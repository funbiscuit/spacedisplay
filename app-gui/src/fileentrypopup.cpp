#include <utility>

#include <QtWidgets>

#include <iostream>
#include "fileentrypopup.h"
#include "spacescanner.h"
#include "filepath.h"
#include "utils.h"
#include "platformutils.h"
#include "FileManager.h"

FileEntryPopup::FileEntryPopup(QWidget *_parent) : parent(_parent) {
    rescanAct = Utils::make_unique<QAction>("Rescan folder", parent);
    rescanAct->setEnabled(false);
    connect(rescanAct.get(), &QAction::triggered, this, &FileEntryPopup::onRescan);

    openInFMAct = Utils::make_unique<QAction>("Open in File Manager", parent);
    connect(openInFMAct.get(), &QAction::triggered, this, &FileEntryPopup::onOpen);

    showInFMAct = Utils::make_unique<QAction>("Show in File Manager", parent);
    connect(showInFMAct.get(), &QAction::triggered, this, &FileEntryPopup::onShow);

    deleteDirAct = Utils::make_unique<QAction>("Delete (recursively)", parent);
    deleteDirAct->setEnabled(false);
    connect(deleteDirAct.get(), &QAction::triggered, this, &FileEntryPopup::onDeleteDir);

    deleteFileAct = Utils::make_unique<QAction>("Delete", parent);
    deleteFileAct->setEnabled(false);
    connect(deleteFileAct.get(), &QAction::triggered, this, &FileEntryPopup::onDeleteFile);

    propertiesAct = Utils::make_unique<QAction>("Properties", parent);
    propertiesAct->setEnabled(false);
    connect(propertiesAct.get(), &QAction::triggered, this, &FileEntryPopup::onProperties);

}

FileEntryPopup::~FileEntryPopup() {}

void FileEntryPopup::updateActions(const SpaceScanner &scanner) {
    rescanAct->setEnabled(true);
    //todo maybe allow to delete while scanning
//    deleteDirAct->setEnabled(scanner->can_refresh());
}

void FileEntryPopup::onRescan() {
    if (onRescanListener)
        onRescanListener(*currentEntryPath);
}

void FileEntryPopup::onDeleteDir() {
    std::cout << "delete dir\n";
}

void FileEntryPopup::onDeleteFile() {
    std::cout << "delete file\n";
}

void FileEntryPopup::onProperties() {
    std::cout << "properties\n";
}

void FileEntryPopup::onShow() {
    FileManager::showFile(currentEntryPath->getPath());
}

void FileEntryPopup::onOpen() {
    FileManager::openFolder(currentEntryPath->getPath());
}

void FileEntryPopup::popup(std::unique_ptr<FilePath> path) {
    currentEntryPath = std::move(path);
    currentEntryName = currentEntryPath->getName();

    QMenu menu(parent);
    auto title = menu.addAction(currentEntryName.c_str());
    title->setEnabled(false);
    menu.addSeparator();
    if (currentEntryPath->isDir()) {
        menu.addAction(rescanAct.get());
        menu.addAction(openInFMAct.get());
        menu.addAction(deleteDirAct.get());
    } else {
        menu.addAction(showInFMAct.get());
        menu.addAction(deleteFileAct.get());
    }
    menu.addAction(propertiesAct.get());

    menu.exec(QCursor::pos() + QPoint(10, 10));
}

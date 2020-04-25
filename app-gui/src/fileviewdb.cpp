#include "fileviewdb.h"

#include "filepath.h"
#include "filedb.h"
#include "fileentry.h"
#include "fileentryview.h"


void FileViewDB::setFileDB(std::shared_ptr<FileDB> db_)
{
    db=std::move(db_);
    rootFile.reset();
}

bool FileViewDB::update(bool includeUnknown, bool includeAvailable)
{
    if(!viewPath || !db || !db->isReady())
        return false;
    FileEntryView::ViewOptions options;
    options.nestLevel = viewDepth;

    uint64_t totalSpace, usedSpace, freeSpace, unknownSpace;

    db->getSpace(usedSpace, freeSpace, totalSpace);
    unknownSpace = totalSpace - freeSpace - usedSpace; //db guarantees this to be >=0
    totalSpace = 0; //we will calculate it manually depending on what is included

    if(includeAvailable)
    {
        options.freeSpace = freeSpace;
        totalSpace+=freeSpace;
    }
    if(includeUnknown)
    {
        options.unknownSpace = unknownSpace;
        totalSpace+=unknownSpace;
    }

    bool hasChanges = false;

    db->processEntry(*viewPath, [this, &totalSpace, &options, &hasChanges](const FileEntry& entry){
        if(!entry.isRoot())
        {
            totalSpace=0;
            //we don't show unknown and free space if child is viewed
            options.freeSpace = 0;
            options.unknownSpace = 0;
        }

        float minSizeRatio = (7.f*7.f)/float(viewRect.h*viewRect.w);

        totalSpace+=entry.get_size();

        options.minSize = int64_t(float(totalSpace)*minSizeRatio);
        if(rootFile)
            FileEntryView::updateView(rootFile, &entry, options);
        else
            rootFile = FileEntryView::createView(&entry, options);
        //TODO check if anything actually changed?
        hasChanges = true;
    });

    // rootFile should be valid if lambda was called but in case something went wrong
    if(hasChanges && rootFile)
        rootFile->allocate_view(viewRect, (textHeight * 3) / 2);

    return hasChanges;
}

void FileViewDB::onThemeChanged()
{
    // just reset pointer to root so on next update it will create new pixmaps with proper color
    rootFile.reset();
}

void FileViewDB::setViewArea(Utils::RectI rect)
{
    viewRect = rect;
}

void FileViewDB::setViewPath(const FilePath& filepath)
{
    viewPath = Utils::make_unique<FilePath>(filepath);
}

void FileViewDB::setViewDepth(int depth)
{
    viewDepth = depth;
}

void FileViewDB::setTextHeight(int height)
{
    textHeight = height;
}

uint64_t FileViewDB::getFilesSize() const
{
    if(isReady() && rootFile)
        return rootFile->get_size();
    return 0;
}

bool FileViewDB::processEntry(const std::function<void(FileEntryView &)> &func)
{
    if(!isReady() || !rootFile)
        return false;

    func(*rootFile);

    return true;
}

FileEntryView* FileViewDB::getHoveredView(int mouseX, int mouseY)
{
    if(!isReady() || !rootFile)
        return nullptr;

    return rootFile->getHoveredView(mouseX, mouseY);
}

bool FileViewDB::isReady() const
{
    return db != nullptr;
}

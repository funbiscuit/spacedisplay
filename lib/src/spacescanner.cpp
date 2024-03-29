
#include "spacescanner.h"
#include "FileIterator.h"
#include "fileentry.h"
#include "filepath.h"
#include "filedb.h"
#include "spacewatcher.h"
#include "platformutils.h"
#include "logger.h"
#include "utils.h"

#include <iostream>
#include <chrono>

SpaceScanner::SpaceScanner(const std::string &path) :
        scannerStatus(ScannerStatus::IDLE), runWorker(true), isMountScanned(false),
        watcherLimitExceeded(false) {

    auto cantScanMsg = Utils::strFormat("Can't open %s", path.c_str());
    if (!PlatformUtils::can_scan_dir(path)) {
        throw std::runtime_error(cantScanMsg);
    }

    //update info about mount points
    availableRoots = PlatformUtils::getAvailableMounts();
    excludedMounts = PlatformUtils::getExcludedPaths();

    try {
        db = Utils::make_unique<FileDB>(path);
    } catch (std::exception &) {
        std::cout << "Can't set as root: " << path << "\n";
        throw std::runtime_error(cantScanMsg);
    }

    watcherLimitExceeded = false;
    try {
        watcher = SpaceWatcher::create(path);
    } catch (std::runtime_error &) { }

    scannerStatus = ScannerStatus::SCANNING;

    isMountScanned = Utils::in_array(path, availableRoots);

    //this will load known info about disk space (available and total) to database
    updateDiskSpace();

    addToQueue(Utils::make_unique<FilePath>(db->getRootPath()), true);

    //Start thread after everything is initialized
    workerThread = std::thread(&SpaceScanner::worker_run, this);
}

SpaceScanner::~SpaceScanner() {
    runWorker = false;
    scannerStatus = ScannerStatus::STOPPING;
    workerThread.join();
}

void SpaceScanner::checkForEvents() {
    if (!watcher)
        return;
    //should be called with locked scan mutex

    while (auto event = watcher->popEvent()) {
        if (event->parentpath.empty())
            continue;

        //try constructing FilePath and add it to queue
        try {
            auto path = Utils::make_unique<FilePath>(event->parentpath,
                                                     db->getRootPath().getRoot());
            addToQueue(std::move(path), false);
            scannerStatus = ScannerStatus::SCANNING;
        } catch (std::exception &) {
            std::cerr << "Unable to construct path from: " << event->parentpath << "\n";
        }
    }
}

void SpaceScanner::worker_run() {
    std::cout << "Start worker thread\n";
    while (runWorker) {
        std::unique_lock<std::mutex> scanLock(scanMtx, std::defer_lock);
        scanLock.lock();

        using namespace std::chrono;
        while (scanQueue.empty() && scannerStatus == ScannerStatus::IDLE) {
            scanLock.unlock();
            std::this_thread::sleep_for(milliseconds(20));
            scanLock.lock();
            checkForEvents();
        }

        //scan queue is not empty at this point
        auto start = high_resolution_clock::now();
        // temporary queue for entries so we only lock mutex to add reasonable number of entries
        // instead of locking for each entry
        std::vector<std::unique_ptr<FileEntry>> scannedEntries;
        std::vector<std::unique_ptr<FilePath>> newPaths;

        updateDiskSpace();
        bool scannedRecursively = false;
        while (!scanQueue.empty() && scannerStatus != ScannerStatus::IDLE) {
            auto scanRequest = std::move(scanQueue.front());
            scanQueue.pop_front();
            // update current scanned path with new data
            if (currentScannedPath)
                *currentScannedPath = *scanRequest.path;
            else
                currentScannedPath = Utils::make_unique<FilePath>(*scanRequest.path);
            scanLock.unlock();
            if (scanRequest.recursive)
                scannedRecursively = true;

            if (watcher) {
                if (watcher->addDir(scanRequest.path->getPath()) == SpaceWatcher::AddDirStatus::DIR_LIMIT_REACHED) {
                    //dir was not added, should report this
                    watcherLimitExceeded = true;
                }
            }

            // if we should perform recursive scan, store all paths to dirs in vector
            scanChildrenAt(*scanRequest.path, scannedEntries,
                           scanRequest.recursive ? &newPaths : nullptr);
            // after scan, all new paths, that should be scanned, are already added to queue
            // so we should add all new entries to database, otherwise we might not be able
            // to find them on the next iteration

            if (scannerStatus == ScannerStatus::STOPPING) {
                //lock mutex because next action will be clearing of queue and it should be protected
                scanLock.lock();
                break;
            }

            // if this is not a recursive scan, we need to store paths to all new dirs
            db->setChildrenForPath(*scanRequest.path, std::move(scannedEntries),
                                   scanRequest.recursive ? nullptr : &newPaths);

            while (scannerStatus == ScannerStatus::SCAN_PAUSED) {
                //if scan is paused, just wait until it isn't
                std::this_thread::sleep_for(milliseconds(20));
            }

            scanLock.lock();
            if (!newPaths.empty()) {
                addChildrenToQueue(newPaths, true, false);
                newPaths.clear();
            }
            checkForEvents();
        }
        updateDiskSpace();
        scanQueue.clear();
        currentScannedPath = nullptr;
        scanLock.unlock();

        auto stop = high_resolution_clock::now();
        auto mseconds = duration_cast<milliseconds>(stop - start).count();
        //TODO this outputs very frequently when watching for changes
        auto msg = Utils::strFormat("Time taken: %dms. Stat: %d files, %d dirs",
                                    mseconds, db->getFileCount(), db->getDirCount());
        if (logger && scannedRecursively)
            logger->log(msg, "SCAN");
        scannerStatus = ScannerStatus::IDLE;
    }

    std::cout << "End worker thread\n";
}

void SpaceScanner::scanChildrenAt(const FilePath &path,
                                  std::vector<std::unique_ptr<FileEntry>> &scannedEntries,
                                  std::vector<std::unique_ptr<FilePath>> *newPaths) {
    auto pathStr = path.getPath();

    //TODO add check if iterator was constructed and we were able to open path
    for (auto it = FileIterator::create(pathStr); it->isValid(); ++(*it)) {
        bool doScan = it->isDir();
        std::unique_ptr<FilePath> entryPath;
        // this section is important for linux since not any path should be scanned (e.g. /proc or /sys)
        if (doScan && scannerStatus != ScannerStatus::STOPPING) {
            std::string newPath = pathStr;
            if (newPath.back() != PlatformUtils::filePathSeparator)
                newPath.push_back(PlatformUtils::filePathSeparator);
            newPath.append(it->getName());
            if (newPath.back() != PlatformUtils::filePathSeparator)
                newPath.push_back(PlatformUtils::filePathSeparator);
            if (Utils::in_array(newPath, availableRoots) || Utils::in_array(newPath, excludedMounts)) {
                doScan = false;
                if (logger) {
                    auto msg = Utils::strFormat("Skip scan of: %s", newPath.c_str());
                    logger->log(msg, "SCAN");
                }
            }
        }
        auto fe = Utils::make_unique<FileEntry>(it->getName(), it->isDir(), it->getSize());
        if (doScan && newPaths) {
            entryPath = Utils::make_unique<FilePath>(path);
            entryPath->addDir(it->getName(), fe->getNameCrc());
            //TODO it would be faster to save all adding for later and then just check if
            // parent path exist in entry (or any of its child paths)
            // if it is not (and most of the times this will be true), we can just push all
            // these paths without further checking
            // but this will give a small improvement (around 1sec when scanning C:\\
            // which takes 15 seconds to scan)
            newPaths->push_back(std::move(entryPath));
        }
        scannedEntries.push_back(std::move(fe));
    }
}

void SpaceScanner::addChildrenToQueue(std::vector<std::unique_ptr<FilePath>> &paths, bool recursiveScan, bool toBack) {
    //create parent for children
    FilePath parent = *paths.front();
    parent.goUp();

    bool safeToBatchAdd = true;
    auto it = scanQueue.begin();

    while (it != scanQueue.end()) {
        auto res = parent.compareTo(*(*it).path);

        if (res == FilePath::CompareResult::DIFFERENT)
            ++it;
        else if (res == FilePath::CompareResult::CHILD && it->recursive)
            //if parent of entries is child to some recursive scan request, then we don't need to add
            return;
        else {
            safeToBatchAdd = false;
            break;
        }
    }
    if (safeToBatchAdd) {
        for (auto &child : paths) {
            ScanRequest request;
            request.path = std::move(child);
            request.recursive = recursiveScan;

            if (toBack)
                scanQueue.push_back(std::move(request));
            else
                scanQueue.push_front(std::move(request));
        }
    } else {
        // Adding all children separately
        // most of the times this will not happen
        for (auto &child : paths)
            addToQueue(std::move(child), recursiveScan, toBack);
    }

}

void SpaceScanner::addToQueue(std::unique_ptr<FilePath> path, bool recursiveScan, bool toBack) {
    auto it = scanQueue.begin();

    while (it != scanQueue.end()) {
        auto res = path->compareTo(*(*it).path);

        if (res == FilePath::CompareResult::DIFFERENT || res == FilePath::CompareResult::CHILD)
            ++it;
        else if (res == FilePath::CompareResult::PARENT)
            //if we are parent path to some path in queue, then we should remove it from queue
            it = scanQueue.erase(it);
        else //equal
        {
            scanQueue.erase(it);
            break;
        }
    }

    ScanRequest request;
    request.path = std::move(path);
    request.recursive = recursiveScan;

    if (toBack)
        scanQueue.push_back(std::move(request));
    else
        scanQueue.push_front(std::move(request));
}

const FileDB& SpaceScanner::getFileDB() const {
    return *db;
}

std::unique_ptr<FilePath> SpaceScanner::getCurrentScanPath() {
    std::lock_guard<std::mutex> lock_mtx(scanMtx);

    if (currentScannedPath) {
        return Utils::make_unique<FilePath>(*currentScannedPath);
    }
    return nullptr;
}

bool SpaceScanner::getWatcherLimits(int64_t &watchedNow, int64_t &watchLimit) {
    if (!watcher)
        return false;
    watchLimit = watcher->getDirCountLimit();
    watchedNow = watcher->getWatchedDirCount();
    if (watchLimit < 0 || !watcherLimitExceeded)
        return false;
    watcherLimitExceeded = false;
    return true;
}

bool SpaceScanner::hasChanges() const {
    return db->hasChanges();
}

bool SpaceScanner::isProgressKnown() const {
    return isMountScanned;
}

int SpaceScanner::getScanProgress() const {
    int64_t totalSpace, usedSpace, freeSpace;
    db->getSpace(usedSpace, freeSpace, totalSpace);
    if ((scannerStatus == ScannerStatus::SCANNING || scannerStatus == ScannerStatus::SCAN_PAUSED)
        && totalSpace > 0) {
        int progress = static_cast<int>((usedSpace * 100) / (totalSpace - freeSpace));
        return Utils::clip(progress, 0, 100);
    }
    return 100;
}

bool SpaceScanner::pauseScan() {
    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    if (scannerStatus == ScannerStatus::SCANNING) {
        scannerStatus = ScannerStatus::SCAN_PAUSED;
        return true;
    }
    return false;
}

bool SpaceScanner::resumeScan() {
    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    if (scannerStatus == ScannerStatus::SCAN_PAUSED) {
        scannerStatus = ScannerStatus::SCANNING;
        return true;
    }
    return false;
}

bool SpaceScanner::canPause() {
    return scannerStatus == ScannerStatus::SCANNING;
}

bool SpaceScanner::canResume() {
    return scannerStatus == ScannerStatus::SCAN_PAUSED;
}

void SpaceScanner::stopScan() {
    {
        std::lock_guard<std::mutex> lock_mtx(scanMtx);
        if (scannerStatus != ScannerStatus::STOPPING && scannerStatus != ScannerStatus::IDLE)
            scannerStatus = ScannerStatus::STOPPING;
    }
    //wait until everything is stopped
    while (scannerStatus != ScannerStatus::IDLE)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void SpaceScanner::updateDiskSpace() {
    auto p = db->getRootPath();

    int64_t total, available;

    PlatformUtils::get_mount_space(p.getName(), total, available);
    db->setSpace(total, available);
}

void SpaceScanner::rescanPath(const FilePath &folder_path) {
    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    auto entry = db->findEntry(folder_path);

    if (!entry)
        return;

    scannerStatus = ScannerStatus::SCANNING;
    //update info about mount points
    availableRoots = PlatformUtils::getAvailableMounts();
    excludedMounts = PlatformUtils::getExcludedPaths();

    updateDiskSpace();//disk space might change since last update, so update it again

    // pushing to front so we start rescanning as soon as possible
    addToQueue(Utils::make_unique<FilePath>(folder_path), true, false);
}

void SpaceScanner::getSpace(int64_t &used, int64_t &available, int64_t &total) const {
    db->getSpace(used, available, total);
}

int64_t SpaceScanner::getFileCount() const {
    return db->getFileCount();
}

int64_t SpaceScanner::getDirCount() const {
    return db->getDirCount();
}

const FilePath &SpaceScanner::getRootPath() const {
    return db->getRootPath();
}

void SpaceScanner::setLogger(std::shared_ptr<Logger> logger_) {
    logger = std::move(logger_);
}

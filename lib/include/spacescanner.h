
#ifndef SPACEDISPLAY_SPACESCANNER_H
#define SPACEDISPLAY_SPACESCANNER_H


#include <string>
#include <vector>
#include <mutex>          // std::mutex
#include <queue>
#include <list>
#include <memory>
#include <thread>
#include <atomic>


enum class ScannerStatus {
    IDLE,
    SCANNING,
    STOPPING,
    SCAN_PAUSED,
};

class FileEntry;

class FilePath;

class FileDB;

class SpaceWatcher;

class Logger;

class SpaceScanner {
    struct ScanRequest {
        std::unique_ptr<FilePath> path;
        bool recursive;
    };
public:

    /**
     * Starts scan of selected path
     * If path can't be scanned, throws an exception
     * @param path
     * @throws std::runtime_error if path can't be scanned
     */
    explicit SpaceScanner(const std::string &path);

    ~SpaceScanner();

    void stopScan();

    /**
     * Pauses current scan and returns true if it was paused
     * If no scan is running returns false
     * @return
     */
    bool pauseScan();

    bool resumeScan();

    bool canPause();

    bool canResume();

    void rescanPath(const FilePath &folder_path);

    const FileDB& getFileDB() const;

    std::unique_ptr<FilePath> getCurrentScanPath();

    /**
     * Get current watcher limits
     * @param watchedNow - how many directories are already watched
     * @param watchLimit - limit on how many directories user can watch (-1 if no limit)
     * @return true if limits are exceeded, false otherwise
     */
    bool getWatcherLimits(int64_t &watchedNow, int64_t &watchLimit);

    /**
     * Returns true if it possible to determine scan progress.
     * For example, if we scan partition and can get total occupied space.
     * @return
     */
    bool isProgressKnown() const;

    int getScanProgress() const;

    bool hasChanges() const;

    const FilePath &getRootPath() const;

    void getSpace(int64_t &used, int64_t &available, int64_t &total) const;

    int64_t getFileCount() const;

    int64_t getDirCount() const;

    void setLogger(std::shared_ptr<Logger> logger);

private:
    std::thread workerThread;
    std::atomic<bool> runWorker;
    std::mutex scanMtx;
    std::unique_ptr<FilePath> currentScannedPath;

    // true if we scan mount point so we can get info about how big it should be
    bool isMountScanned;

    //edits to queue should be mutex protected
    std::list<ScanRequest> scanQueue;

    std::vector<std::string> availableRoots;
    /**
     * Important for unix since we can't scan /proc /sys and some others
     */
    std::vector<std::string> excludedMounts;

    /**
     * If valid rootFile is available then this will update info about total and available space on this drive
     * Can be called multiple times, just need rootFile to be valid
     */
    void updateDiskSpace();

    std::atomic<ScannerStatus> scannerStatus;
    std::unique_ptr<FileDB> db;

    std::unique_ptr<SpaceWatcher> watcher;
    std::atomic<bool> watcherLimitExceeded;

    std::shared_ptr<Logger> logger;

    void worker_run();

    void checkForEvents();

    /**
     * Adds specified path to queue. If such path already exist in queue,
     * it will be moved to back (if toBack is true) or to front (otherwise).
     * All paths in queue that contain this path, will be removed from queue
     * This function must be called with locked scan mutex.
     * If there are any path in queue that is contained in this path, then this
     * path will not be added
     * @param path
     * @param toBack
     */
    void addToQueue(std::unique_ptr<FilePath> path, bool recursiveScan, bool toBack = true);

    /**
     * Same as addToQueue but assumes that all paths in provided vector have common parent
     * @param paths
     * @param recursiveScan
     * @param toBack
     */
    void addChildrenToQueue(std::vector<std::unique_ptr<FilePath>> &paths, bool recursiveScan, bool toBack = true);

    /**
     * Performs a scan at given path, creates entry for each child and populates scannedEntries vector
     * Scan is not recursive, only direct children are scanned
     * @param path - FilePath where to perform scan
     * @param scannedEntries - vector reference where to add scanned children
     * @param newPaths - pointer to vector where to add paths to scanned dirs
     */
    void scanChildrenAt(const FilePath &path,
                        std::vector<std::unique_ptr<FileEntry>> &scannedEntries,
                        std::vector<std::unique_ptr<FilePath>> *newPaths = nullptr);
};


#endif //SPACEDISPLAY_SPACESCANNER_H

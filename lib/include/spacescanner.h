
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
    IDLE=0,
    SCANNING=1,
    STOPPING=2,
};

class FileEntry;
class FilePath;
class FileDB;

enum class ScannerError
{
    NONE = 0,
    SCAN_RUNNING,
    CANT_OPEN_DIR
};


class SpaceScanner {

public:

    SpaceScanner();
    ~SpaceScanner();
    ScannerError scan_dir(const std::string &path);

    void stop_scan();

    void rescan_dir(const FilePath& folder_path);

    /**
     * For windows this is a list of available drives (C:\\, D:\\ etc)
     * For linux this is a list of mount points where partitions are mounted (including /)
     * So while scanning specific root, all other mount points will be skipped
     */
    std::vector<std::string> get_available_roots();

    std::shared_ptr<FileDB> getFileDB();


    /**
     * Returns true if it possible to determine scan progress.
     * For example, if we scan partition and can get total occupied space.
     * @return
     */
    bool isProgressKnown() const;

    int get_scan_progress() const;
    bool is_running() const;
    bool is_loaded() const;
    bool can_refresh() const;
    bool has_changes() const;

    const FilePath* getRootPath() const;

    void getSpace(uint64_t& used, uint64_t& available, uint64_t& total) const;

private:
    std::thread workerThread;
    std::atomic<bool> runWorker;
    std::mutex scanMtx;

    // true if we scan mount point so we can get info about how big it should be
    bool isMountScanned;

    //edits to queue should be mutex protected
    std::list<std::unique_ptr<FilePath>> scanQueue;

    std::vector<std::string> availableRoots;
    /**
     * Important for unix since we can't scan /proc /sys and some others
     */
    std::vector<std::string> excludedMounts;

    /**
     * If valid rootFile is available then this will update info about total and available space on this drive
     * Can be called multiple times, just need rootFile to be valid
     */
    void update_disk_space();

    std::atomic<ScannerStatus> scannerStatus;
    std::shared_ptr<FileDB> db;

    void worker_run();

    /**
     * Performs a scan at given path, creates entry for each child and populates scannedEntries vector
     * Scan is not recursive, only direct children are scanned
     * @param path - FilePath where to perform scan
     * @param scannedEntries - vector reference where to add scanned children
     */
    void scanChildrenAt(const FilePath& path,
            std::vector<std::unique_ptr<FileEntry>>& scannedEntries
            );
};


#endif //SPACEDISPLAY_SPACESCANNER_H

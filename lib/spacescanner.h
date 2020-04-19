
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
class FileEntryView;
class FileEntryPool;

enum class ScannerError
{
    NONE = 0,
    SCAN_RUNNING,
    CANT_OPEN_DIR
};


class SpaceScanner {

    struct ScannedEntry {
        std::unique_ptr<FileEntry> entry;
        bool addToQueue;
    };

public:

    SpaceScanner();
    ~SpaceScanner();
    ScannerError scan_dir(const std::string &path);

    void stop_scan();
    void reset_database();

    void rescan_dir(const FilePath& folder_path);

    /**
     * For windows this is a list of available drives (C:\\, D:\\ etc)
     * For linux this is a list of mount points where partitions are mounted (including /)
     * So while scanning specific root, all other mount points will be skipped
     */
    std::vector<std::string> get_available_roots();

    float get_scan_progress();
    void updateEntryView(std::shared_ptr<FileEntryView>& view, float minSizeRatio, uint16_t flags, const FilePath* filepath, int depth);
    bool is_running();
    bool is_loaded();
    bool can_refresh();
    bool has_changes();

    const FilePath* getRootPath() const;

    uint64_t get_total_space();
    uint64_t get_free_space();
    uint64_t get_scanned_space();

private:
    std::thread workerThread;
    std::atomic<bool> runWorker;
    // used to indicate that rootFile is valid
    std::atomic<bool> rootAvailable;

    uint64_t totalSpace = 0;
    uint64_t freeSpace = 0;
    std::atomic<uint64_t> scannedSpace;
    // mutex used to protect access to rootFile and scanQueue
    std::mutex mtx;
    std::unique_ptr<FileEntryPool> entryPool;

    //edits to queue should be mutex protected
    std::list<FileEntry*> scanQueue;

    std::atomic<bool> hasPendingChanges;

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
    std::unique_ptr<FileEntry> rootFile;
    std::unique_ptr<FilePath> rootPath;
    uint64_t fileCount=0;
    uint64_t totalSize=0;

    void worker_run();

    /**
     * Performs a scan at given path, creates entry for each child and populates scannedEntries vector
     * Scan is not recursive, only direct children are scanned
     * @param path - FilePath where to perform scan
     * @param scannedEntries - vector reference where to add scanned children
     */
    void scanChildrenAt(const FilePath& path, std::vector<ScannedEntry>& scannedEntries);

    /**
     * Creates root FileEntry at specified path. When called, existing root and all children should be already deleted
     * @param path - global path to root dir
     * @return true if rootFile is created, false if unsuccessful
     */
    bool create_root_entry(const std::string& path);
};


#endif //SPACEDISPLAY_SPACESCANNER_H

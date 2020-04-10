
#ifndef SPACEDISPLAY_SPACESCANNER_H
#define SPACEDISPLAY_SPACESCANNER_H


#include <string>
#include <vector>
#include <mutex>          // std::mutex
#include <queue>
#include <list>
#include <memory>
#include <thread>

enum class ScannerStatus {
    IDLE=0,
    SCANNING=1,
    STOPPING=2,
};

class FileEntry;
class FileEntryShared;
class FileEntryPool;

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
    void reset_database();

    void rescan_dir(const std::string& folder_path);

    /**
     * For windows this is a list of available drives (C:\\, D:\\ etc)
     * For linux this is a list of mount points where partitions are mounted (including /)
     * So while scanning specific root, all other mount points will be skipped
     */
    std::vector<std::string> get_available_roots();

    float get_scan_progress();
    std::shared_ptr<FileEntryShared> get_root_file(float minSizeRatio, uint16_t flags, const char* filepath, int depth);
    void update_root_file(std::shared_ptr<FileEntryShared>& root, float minSizeRatio, uint16_t flags, const char* filepath, int depth);
    bool is_running();
    bool is_loaded();
    bool can_refresh();
    bool has_changes();

    const char* get_root_path();

    uint64_t get_total_space();
    uint64_t get_free_space();
    uint64_t get_scanned_space();

private:
    std::thread workerThread;
    bool runWorker;

    uint64_t totalSpace = 0;
    uint64_t freeSpace = 0;
    std::mutex mtx;           // mutex for critical section
    std::unique_ptr<FileEntryPool> entryPool;

    //edits to queue should be mutex protected
    std::list<FileEntry*> scanQueue;
    std::queue<std::string> rescanPathQueue;

    bool hasPendingChanges = false;

    void init_platform();
    void on_before_new_scan();
    void cleanup_platform();

    std::vector<std::string> availableRoots;
    /**
     * Important for unix since we can't scan /proc /sys and some others
     */
    std::vector<std::string> excludedMounts;

    /**
     * Scans system for mount points and populates availableRoots and excludedMounts vectors
     * Can be called multiple times to update information about available mounts
     */
    void update_available_drives();

    /**
     * If valid rootFile is available then this will update info about total and available space on this drive
     * Can be called multiple times, just need rootFile to be valid
     */
    void update_disk_space();

    ScannerStatus scannerStatus;
    std::unique_ptr<FileEntry> rootFile;
    uint64_t fileCount=0;
    uint64_t totalSize=0;
    
    FileEntry* getEntryAt(const char* path);

    void worker_run();
    void update_entry_children(FileEntry* entry);

    /**
     * Creates root FileEntry at specified path. When called, existing root and all children should be already deleted
     * @param path - global path to root dir
     * @return true if rootFile is created, false if unsuccessful
     */
    bool create_root_entry(const std::string& path);
};


#endif //SPACEDISPLAY_SPACESCANNER_H

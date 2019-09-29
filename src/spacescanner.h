
#ifndef SPACEDISPLAY_SPACESCANNER_H
#define SPACEDISPLAY_SPACESCANNER_H


#include <string>
#include <vector>
#include <mutex>          // std::mutex
#include <queue>
#include "fileentry.h"
#include "fileentryshared.h"
#include "fileentrypool.h"

enum ScannerStatus {
    IDLE=0,
    SCANNING=1,
    STOPPING=2,
    WATCHING=3,
};

class SpaceScanner {

public:

    SpaceScanner();
    ~SpaceScanner();
    void scan_dir(const std::string &path);

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
    FileEntrySharedPtr get_root_file(float minSizeRatio, uint16_t flags, const char* filepath, int depth);
    void update_root_file(FileEntrySharedPtr& root, float minSizeRatio, uint16_t flags, const char* filepath, int depth);
    bool is_running();
    bool is_watching();
    bool is_loaded();
    bool can_refresh();
    bool has_changes();

    const char* get_root_path()
    {
        if(rootFile)
            return rootFile->get_name();
        return "";
    }

    void lock();
    bool try_lock();
    void unlock();

    uint64_t get_total_space();
    uint64_t get_free_space();
    uint64_t get_scanned_space();

//    type_signal_data_changed signal_data_changed();

private:
//    type_signal_data_changed m_signal_data_changed;
    uint64_t totalSpace;
    uint64_t freeSpace;
    std::recursive_mutex mtx;           // mutex for critical section
    FileEntryPool entryPool;

    std::queue<std::string> rescanPathQueue;

    bool hasPendingChanges = false;

//    sigc::connection timeout_connection;
    bool on_timeout();

    void init_platform();
    void on_before_new_scan();
    void cleanup_platform();

    std::vector<std::string> availableRoots;
    /**
     * Important for unix since we can't scan /proc /sys and some others
     */
    std::vector<std::string> excludedMounts;

    void read_available_drives();

    ScannerStatus scannerStatus;
    FileEntry* rootFile;
    uint64_t fileCount=0;
    uint64_t totalSize=0;
    void scan_dir_prv(FileEntry *parent);
    void async_scan();
    void async_rescan();
    void watch_file_changes();
    void check_disk_space();

    /**
     * Creates root FileEntry at specified path. When called, existing root and all children will be deleted
     * @param path - global path to root dir
     * @return pointer to created FileEntry, nullptr if unsuccessful
     */
    FileEntry* create_root_entry(const char* path);
};


#endif //SPACEDISPLAY_SPACESCANNER_H

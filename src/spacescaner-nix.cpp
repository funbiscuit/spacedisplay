
#ifdef __linux__
#include "spacescanner.h"
#include "fileentry.h"
#include "fileentrypool.h"
#include "utils.h"

#include <regex>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <chrono>
#include <sys/statvfs.h>
#include <unordered_map>
#include <thread>


void SpaceScanner::init_platform()
{
}

void SpaceScanner::cleanup_platform()
{
}

void SpaceScanner::on_before_new_scan()
{

}

#endif


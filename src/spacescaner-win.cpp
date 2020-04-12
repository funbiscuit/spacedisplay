
#ifdef _WIN32
// windows code goes here
#include "spacescanner.h"
#include "fileentry.h"
#include "fileentrypool.h"
#include "utils.h"
#include "platformutils.h"

#include <fstream>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <winbase.h>

void SpaceScanner::on_before_new_scan()
{

}

void SpaceScanner::init_platform()
{

}

void SpaceScanner::cleanup_platform()
{
}

#endif

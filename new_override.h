// A class for reporting memory leak when it is destructed
 
#pragma once 
#include <array>

struct LeakReporter
{
    enum class BoundaryDetectMode
    {
        DETECT_NO_ACCESS_DETECTION,
        DETECT_OVERFLOW,
        DETECT_UNDERFLOW,
        DETECT_ACCESS_AFTER_DELETE,
    };

    LeakReporter(const char* file_path);
    ~LeakReporter();

    static void SetDetectMode(BoundaryDetectMode detect_mode);

    static std::array<char, 256> file_path;
    static bool already_created;
    static BoundaryDetectMode detect_mode;
};


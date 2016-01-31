// Implementation of new override

#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <array>
#include <cstring>
#include <algorithm>
#include <utility>
#include <fstream>
#include <cstdint>
#include <numeric>
#include <exception>
#include <iostream>

#include "new_override.h"
#include "free_allocator.h"
#include "assertion.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#pragma warning(push)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(pop)

bool g_forced_usual_alloc = false;

// record memory allocation information
struct MemRecord
{
    MemRecord(DWORD return_addr, unsigned alloc_idx, bool is_new_array)
        :return_addr(return_addr), alloc_idx(alloc_idx), is_new_array(is_new_array)
    {}

    bool operator < (const MemRecord& rhs) const
    {
        return alloc_idx < rhs.alloc_idx;
    }

    DWORD return_addr;
    unsigned alloc_idx;
    bool is_new_array;
};

// defined custom unordered map type
template<typename T, typename Data>
using free_unordered_map = std::unordered_map < T, Data, std::hash<T>, std::equal_to<T>, free_allocator<std::pair<T, Data>> > ;

// defined custom vector type
template<typename T>
using free_vec = std::vector < T, free_allocator<T>> ;

// global records
static free_unordered_map<void*, MemRecord> g_allocation_map;
static free_unordered_map<void*, MemRecord> g_freed_set;
static unsigned g_allocation_number = 0;

const unsigned k_page_size = 4096;

static void* PageAlignedAllocate(size_t size, bool detect_overflow) 
{
    bool dummy_allocate = false;
    if (size == 0)
    {
        dummy_allocate = true;
        size = 4096;
    }

    const unsigned page_init_offset = detect_overflow ? 0 : k_page_size;
    unsigned base_page_num = size / k_page_size;
    if (size > base_page_num * k_page_size)
        ++base_page_num;

    const unsigned return_addr_offset = detect_overflow ? (k_page_size * base_page_num - size) : 0;
    void* p = VirtualAlloc(0, k_page_size * (base_page_num + 1), MEM_RESERVE, PAGE_NOACCESS);
    if (p)
    {
        p = VirtualAlloc((unsigned char*)p + page_init_offset, base_page_num * k_page_size, MEM_COMMIT, PAGE_READWRITE);
        return (unsigned char*)p + return_addr_offset;
    }
    else if (dummy_allocate)
        return p;
    else
        return nullptr;
}

static void PageAlignedFree(void* ptr, bool detect_overflow)
{
    VirtualFree(ptr, 0, MEM_DECOMMIT);
}

static void* alloc_it(size_t size)
{
    switch (LeakReporter::detect_mode)
    {
    case LeakReporter::BoundaryDetectMode::DETECT_OVERFLOW:
    case LeakReporter::BoundaryDetectMode::DETECT_ACCESS_AFTER_DELETE:
        return PageAlignedAllocate(size, true);
    case LeakReporter::BoundaryDetectMode::DETECT_UNDERFLOW:
        return PageAlignedAllocate(size, false);
    default:
        return std::malloc(size);
    }
}

static void dealloc_it(void* ptr)
{
    switch (LeakReporter::detect_mode)
    {
    case LeakReporter::BoundaryDetectMode::DETECT_OVERFLOW:
    case LeakReporter::BoundaryDetectMode::DETECT_ACCESS_AFTER_DELETE:
        PageAlignedFree(ptr, true);
        break;
    case LeakReporter::BoundaryDetectMode::DETECT_UNDERFLOW:
        PageAlignedFree(ptr, false);
        break;
    default:
        std::free(ptr);
        break;
    }

}

// common allocation function
static void* common_allocation_helper(size_t size, bool is_new_array, decltype(_ReturnAddress()) returned_addr)
{
    void* allocated;
    if (!g_forced_usual_alloc)
    {
        allocated = alloc_it(size);

        if (!allocated)
            throw std::bad_alloc();

        g_allocation_map.emplace(allocated, MemRecord((DWORD)returned_addr, g_allocation_number++, is_new_array));

        auto iter = g_freed_set.find(allocated);
        if (iter != g_freed_set.end())
            g_freed_set.erase(iter);
    }
    else
        allocated = std::malloc(size);

    return allocated;
}

// common deallocation function
static void common_deallocation_helper(void* ptr, bool is_array_delete)
{
    if (!g_forced_usual_alloc)
    {
        if (!ptr) return;

        auto iter = g_allocation_map.find(ptr);
        if (iter == g_allocation_map.end())
        {
            if (g_freed_set.find(ptr) != g_freed_set.end())
                ASSERT_ALWAYS(false, "Double delete!");
            else
                ASSERT_ALWAYS(false, "Trying to delete non-heap pointer.");
        }
        else if (iter->second.is_new_array != is_array_delete)
            if (is_array_delete)
                ASSERT_ALWAYS(false, "Mismatched new/delete[] pair.");
            else
                ASSERT_ALWAYS(false, "Mismatched new[]/delete pair.");
        else
        {
            g_freed_set.emplace(iter->first, iter->second);
            g_allocation_map.erase(iter);
            dealloc_it(ptr);
        }
    }
    else
        std::free(ptr);
}

void* operator new(size_t size)
{
    return common_allocation_helper(size, false, _ReturnAddress());
}

void* operator new[](size_t size)
{
    return common_allocation_helper(size, true, _ReturnAddress());
}

void operator delete(void* ptr)
{
    common_deallocation_helper(ptr, false);
}

void operator delete[](void* ptr)
{
    common_deallocation_helper(ptr, true);
}

void* operator new(size_t size, const std::nothrow_t&)
{
    try { return common_allocation_helper(size, false, _ReturnAddress()); }
    catch (std::bad_alloc) { return nullptr; }
}

void* operator new[](size_t size, const std::nothrow_t&)
{
    try { return common_allocation_helper(size, true, _ReturnAddress()); }
    catch (std::bad_alloc) { return nullptr; }
}

void operator delete(void* ptr, const std::nothrow_t&)
{
    common_deallocation_helper(ptr, false);
}

void operator delete[](void* ptr, const std::nothrow_t&)
{
    common_deallocation_helper(ptr, true);
}


LeakReporter::LeakReporter(const char* report_path)
{
    ASSERT_ALWAYS(!already_created, "Cannot create multiple leak reporter!");
    already_created = true;

    std::copy(report_path, report_path + std::strlen(report_path) + 1, file_path.begin());
}

using line_num_to_times = free_unordered_map<decltype(IMAGEHLP_LINE::LineNumber), unsigned> ;
using statistics_map = free_unordered_map < decltype(IMAGEHLP_LINE::FileName), line_num_to_times > ;
static void report_and_fill_in_statistics(std::ostream& ost, statistics_map& statistics, const free_vec<MemRecord>& mem_list)
{
    // get symbol info
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = {0};
    PSYMBOL_INFO func_info = (PSYMBOL_INFO)buffer;
    func_info->SizeOfStruct = sizeof(SYMBOL_INFO);
    func_info->MaxNameLen = MAX_SYM_NAME;
    func_info->Flags = SYMFLAG_FUNCTION;

    // initilaize line info
    IMAGEHLP_LINE line_info;
    DWORD dwdisplacement = 0;
    line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE);

    for (auto& report_item : mem_list)
    {
        // get info about function name
        auto result = SymFromAddr(GetCurrentProcess(), report_item.return_addr, nullptr, func_info);
        ASSERT_ALWAYS(result, "failed to load symbol from address");

        // get info about line number and file name
        auto result2 = SymGetLineFromAddr(GetCurrentProcess(), report_item.return_addr, &dwdisplacement, &line_info);
        ASSERT_ALWAYS(result2, "failed to load symbol from address");

        // output info
        ost << line_info.FileName << " -> line " << line_info.LineNumber << " -> function: "<< func_info->Name << "\n";
        auto iter = statistics.find(line_info.FileName);
        if (iter == statistics.end())
            iter = statistics.emplace(line_info.FileName, line_num_to_times{{line_info.LineNumber, 0}}).first;

        // update leak count
        ++iter->second[line_info.LineNumber];
    }
}

static unsigned report_statistics(std::ostream& ost, const statistics_map& statistics, const char* unit_name)
{
    // output statistics to ost
    const auto k_max_star_num = 30U;
    unsigned num_in_program = 0;
    for (auto& file_count : statistics)
    {
        // output statistics per file
        unsigned num_in_file = 0;
        ost << file_count.first << "\n";
        for (auto& line_count : file_count.second)
        {
            ost << "\tLine " << line_count.first << " -\t" << line_count.second << " " << unit_name << "\t";
            std::fill_n(std::ostream_iterator<char>(ost), std::min(line_count.second, k_max_star_num), '*');
            ost << "\n";
            num_in_file += line_count.second;
            num_in_program += line_count.second;
        }
        ost << "\tTotal " << unit_name << " in file: " << num_in_file << " " << unit_name << "\t";
        std::fill_n(std::ostream_iterator<char>(ost), std::min(num_in_file, k_max_star_num), '*');
        ost << "\n";
    }
    // output whole program statistics
    ost << "Total "<< unit_name << " in program: " << num_in_program << " " << unit_name << "\n";
    return num_in_program;
}

LeakReporter::~LeakReporter()
{
    g_forced_usual_alloc = true;

    // sort leaks based on its allocation time
    free_vec<MemRecord> unfreed_list;
    free_vec<MemRecord> alloc_list; // for allocation statistics
    for (auto& mem_pair : g_allocation_map)
    {
        unfreed_list.push_back(mem_pair.second);
        alloc_list.push_back(mem_pair.second);
    }

    // already freed memory
    for (auto& mem_pair : g_freed_set)
    {
        alloc_list.push_back(mem_pair.second);
    }

    std::sort(unfreed_list.begin(), unfreed_list.end());
    std::sort(alloc_list.begin(), alloc_list.end());

    // initialize symbols
    auto result = SymInitialize(GetCurrentProcess(), nullptr, true);
    ASSERT_ALWAYS(result, "Failed to initialize symbol");

    // define file statistics map
    statistics_map unfreed_statistics;
    statistics_map alloc_statistics;

    // print leaks info to leak_log
    std::ofstream leak_log(file_path.data());
    ASSERT_ALWAYS(leak_log, "Failed to create leak report file");

    unsigned num_leaks = 0;
    if (!unfreed_list.empty())
    {
        // report leak
        leak_log << "==============================\n";
        leak_log << "Leak report:\n";
        leak_log << "==============================\n";
        report_and_fill_in_statistics(leak_log, unfreed_statistics, unfreed_list);

        leak_log << "\n==============================\n";
        leak_log << "Leak Statistics:\n";
        leak_log << "==============================\n";
        num_leaks = report_statistics(leak_log, unfreed_statistics, "leaks");
    }

    leak_log << "\n==============================\n";
    leak_log << "Allocation Statistics:\n";
    leak_log << "==============================\n";
    std::cout.setstate(std::ios_base::badbit);
    report_and_fill_in_statistics(std::cout, alloc_statistics, alloc_list);
    std::cout.clear();
    report_statistics(leak_log, alloc_statistics, "allocs");

    SymCleanup(GetCurrentProcess());
    if (!g_allocation_map.empty())
    {
      std::cerr << num_leaks << " memory leaks detected. check" << file_path.data() << " for more information." << std::endl;
      std::cerr << "Press return to continue." << std::endl;
      std::cin.get();
    }
}

void LeakReporter::SetDetectMode(BoundaryDetectMode detect_mode_)
{
    ASSERT_ALWAYS(!already_created, "Should not change detect mode after reporter created!");
    detect_mode = detect_mode_;
}

std::array<char, 256> LeakReporter::file_path;
LeakReporter::BoundaryDetectMode LeakReporter::detect_mode = 
    LeakReporter::BoundaryDetectMode::DETECT_NO_ACCESS_DETECTION;
bool LeakReporter::already_created = false;

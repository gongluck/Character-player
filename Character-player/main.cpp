#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#define ssize_t SSIZE_T
#endif
#include <vlc/vlc.h>

#include "../../Code-snippet/cpp/process/process.h"

#define CHECKEQUALRET(ret, compare)\
if(ret == compare)\
{\
    std::cerr << "error ocurred in " << __FILE__\
              << "`s line " << __LINE__\
              << ", error " << ret;\
    goto END;\
}

#define CHECKNEQUALRET(ret, compare)\
if(ret != compare)\
{\
    std::cerr << "error ocurred in " << __FILE__\
              << "`s line " << __LINE__\
              << ", error " << ret;\
    goto END;\
}

static HWND WND = nullptr;
static int WIDTH = 1920;
static int HEIGHT = 1080;
static char* out_buffer = nullptr;
static char* tmp_buffer = nullptr;
static char* print_buffer = nullptr;
static void* lock(void* data, void** p_pixels)
{
    *p_pixels = out_buffer;
    return 0;
}

static void unlock(void* data, void* id, void* const* p_pixels)
{
}

static void display(void* data, void* id)
{
    auto bitmap = ::CreateBitmap(WIDTH, HEIGHT, 1, 32, out_buffer);
    auto wndDC = ::GetDC(WND);
    auto memDC = ::CreateCompatibleDC(wndDC);
    auto oldbitmap = static_cast<HBITMAP>(::SelectObject(memDC, bitmap));

    auto size = ::GetDeviceCaps(wndDC, LOGPIXELSY);
    auto is = ::IsProcessDPIAware();
    auto set = SetProcessDPIAware();
    ::RECT rect = { 0 };
    ::GetClientRect(WND, &rect);
    ::StretchBlt(wndDC, 0, 0, rect.right-rect.left, rect.bottom-rect.top, memDC, 0, 0, WIDTH, HEIGHT, SRCCOPY);

    ::SelectObject(memDC, oldbitmap);
    ::DeleteObject(bitmap);
    ::DeleteDC(memDC);
    ::ReleaseDC(WND, wndDC);
}

int getwindows(const TCHAR* classname,
    const gprocess::WindowInfo& info,
    std::vector<std::shared_ptr<gprocess::WindowInfo>>& result)
{
    std::vector<std::shared_ptr<gprocess::WindowInfo>> tmp;
    TCHAR classname_[MAX_PATH] = { 0 };
    for (int i = 0; i < info.childs.size(); ++i)
    {
        GetClassName(reinterpret_cast<HWND>(info.childs[i]->window), classname_, _countof(classname_));
        if (_tcscmp(classname_, classname) == 0)
        {
            tmp.push_back(info.childs[i]);
        }
    }
    for (const auto& each : tmp)
    {
        result.push_back(each);
    }
    return 0;
}
int getwindows(const TCHAR* classname,
    const std::vector<std::shared_ptr<gprocess::WindowInfo>>& windows,
    std::vector<std::shared_ptr<gprocess::WindowInfo>>& result)
{
    std::vector<std::shared_ptr<gprocess::WindowInfo>> tmp;
    for (int i = 0; i < windows.size(); ++i)
    {
        getwindows(classname, *windows[i], tmp);
    }
    for (const auto& each : tmp)
    {
        result.push_back(each);
    }
    return 0;
}

int main(int argc, char** argv)
{
    libvlc_instance_t* inst_ = nullptr;
    libvlc_media_t* media_ = nullptr;
    libvlc_media_player_t* player_ = nullptr;
    libvlc_media_list_t* list_ = nullptr;
    libvlc_media_list_player_t* plist_ = nullptr;
    HWND wnd_ = nullptr;

    std::vector<gprocess::ProcessInfo> result;
    gprocess::WindowInfo windowinfo;
    std::vector<std::shared_ptr<gprocess::WindowInfo>> windows;
    std::vector<std::shared_ptr<gprocess::WindowInfo>> tmpwindows;
    auto ret = gprocess::gethandle("Taskmgr.exe", result);
    CHECKNEQUALRET(ret, 0);
    windowinfo.processid = result[0].processid;
    ret = gprocess::getallwindows(&windowinfo);
    CHECKNEQUALRET(ret, 0);

    ret = getwindows(TEXT("TaskManagerWindow"), windowinfo, windows);
    CHECKNEQUALRET(ret, 0);
    ret = getwindows(TEXT("NativeHWNDHost"), windows, tmpwindows);
    CHECKNEQUALRET(ret, 0);
    windows.clear();
    windows.swap(tmpwindows);
    ret = getwindows(TEXT("DirectUIHWND"), windows, tmpwindows);
    CHECKNEQUALRET(ret, 0);
    windows.clear();
    windows.swap(tmpwindows);
    ret = getwindows(TEXT("CvChartWindow"), windows, tmpwindows);
    CHECKNEQUALRET(ret, 0);
    CHECKEQUALRET(tmpwindows.size(), 0);

    wnd_ = reinterpret_cast<HWND>(tmpwindows[tmpwindows.size() - 1]->window);
    //wnd_ = (HWND)0x000E0786;

    out_buffer = static_cast<char*>(malloc(HEIGHT * WIDTH * 4));
    CHECKEQUALRET(out_buffer, nullptr);

    inst_ = libvlc_new(0, nullptr);
    CHECKEQUALRET(inst_, nullptr);
    media_ = libvlc_media_new_path(inst_, argc <= 1 ? "badapple.mp4" : argv[1]);
    CHECKEQUALRET(media_, nullptr);

    //player_ = libvlc_media_player_new_from_media(media_);
    player_ = libvlc_media_player_new(inst_);
    CHECKEQUALRET(player_, nullptr);

    libvlc_video_set_callbacks(player_, lock, unlock, display, 0);
    libvlc_video_set_format(player_, "RGBA", WIDTH, HEIGHT, WIDTH * 4);
    //libvlc_media_player_set_hwnd(player_, reinterpret_cast<void*>(wnd_));
    WND = wnd_;

    // play loop
    list_ = libvlc_media_list_new(inst_);
    plist_ = libvlc_media_list_player_new(inst_);
    libvlc_media_list_add_media(list_, media_);
    libvlc_media_list_player_set_media_list(plist_, list_);
    libvlc_media_list_player_set_media_player(plist_, player_);
    libvlc_media_list_player_set_playback_mode(plist_, libvlc_playback_mode_loop);
    libvlc_media_list_player_play(plist_);

    std::cin.get();

END:
    if (player_ != nullptr)
    {
        libvlc_media_player_stop(player_);
        libvlc_media_player_release(player_);
        player_ = nullptr;
    }
    if (media_ != nullptr)
    {
        libvlc_media_release(media_);
        media_ = nullptr;
    }

    if (plist_ != nullptr)
    {
        libvlc_media_list_player_release(plist_);
        plist_ = nullptr;
    }
    if (list_ != nullptr)
    {
        libvlc_media_list_release(list_);
        list_ = nullptr;
    }
    if (inst_ != nullptr)
    {
        libvlc_release(inst_);
        inst_ = nullptr;
    }

    if (out_buffer != nullptr)
    {
        free(out_buffer);
        out_buffer = nullptr;
    }
}
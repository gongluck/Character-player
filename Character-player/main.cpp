#include <iostream>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <Windows.h>
#define ssize_t SSIZE_T
#endif
#include <vlc/vlc.h>

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

static int WIDTH = 200;
static int HEIGHT = 50;
static char* out_buffer = nullptr;
static char* print_buffer = nullptr;
std::atomic<bool> atomiclock;
bool gotframe = false;
static void* lock(void* data, void** p_pixels)
{
    while (atomiclock)
        ;
    atomiclock = true;
    *p_pixels = out_buffer;
    return 0;
}
static void unlock(void* data, void* id, void* const* p_pixels)
{
    atomiclock = false;
    gotframe = true;
}
static void display(/*void* data, void* id*/)
{
    if (!gotframe)
    {
        return;
    }

    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { 0 };
    SetConsoleCursorPosition(hConsoleOutput, pos);

    RGBQUAD* rgba = reinterpret_cast<RGBQUAD*>(out_buffer);
    for (int i = 0; i < HEIGHT; ++i)
    {
        for (int j = 0; j < WIDTH; ++j)
        {
            auto point = rgba[WIDTH * i + j];
            auto light = (point.rgbRed + point.rgbGreen + point.rgbBlue) / 3;
            print_buffer[i * (WIDTH + 2) + j] = light > 127 ? '*' : ' ';
        }
        print_buffer[i * (WIDTH + 2) + WIDTH] = '\r';
        print_buffer[i * (WIDTH + 2) + WIDTH + 1] = '\n';
    }
    puts(print_buffer);
}

int main(int argc, char** argv)
{
    libvlc_instance_t* inst_ = nullptr;
    libvlc_media_t* media_ = nullptr;
    libvlc_media_player_t* player_ = nullptr;
    libvlc_media_list_t* list_ = nullptr;
    libvlc_media_list_player_t* plist_ = nullptr;
    int ret = 0;

    std::cout << "Usage: Character-player [media file] [out width] [out height]" << std::endl;

    if (argc >= 4)
    {
        WIDTH = atoi(argv[2]);
        HEIGHT = atoi(argv[3]);
    }

    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD size = { WIDTH + 4, HEIGHT + 5 };
    SetConsoleScreenBufferSize(hConsoleOutput, size);
    SMALL_RECT rc = { 0,0, WIDTH + 1, HEIGHT + 1 };
    SetConsoleWindowInfo(hConsoleOutput, true, &rc);

    libvlc_log_close(nullptr);

    bool exit = false;
    std::thread th([&]()
    {
        while (!exit)
        {
            while (atomiclock)
                ;
            atomiclock = true;
            display();
            atomiclock = false;
        }
    });

    inst_ = libvlc_new(0, nullptr);
    CHECKEQUALRET(inst_, nullptr);
    media_ = libvlc_media_new_path(inst_, argc <= 1 ? "badapple.mp4" : argv[1]);
    CHECKEQUALRET(media_, nullptr);

    out_buffer = static_cast<char*>(malloc(HEIGHT * WIDTH * 4));
    CHECKEQUALRET(out_buffer, nullptr);
    print_buffer = static_cast<char*>(malloc(HEIGHT * (WIDTH + 2) + 1));
    CHECKEQUALRET(print_buffer, nullptr);
    print_buffer[HEIGHT * (WIDTH + 2)] = '\0';

    //player_ = libvlc_media_player_new_from_media(media_);
    player_ = libvlc_media_player_new(inst_);
    CHECKEQUALRET(player_, nullptr);

    libvlc_video_set_callbacks(player_, lock, unlock, nullptr, 0);
    libvlc_video_set_format(player_, "RGBA", WIDTH, HEIGHT, WIDTH * 4);
    //libvlc_media_player_set_hwnd(player_, GetDesktopWindow());
    
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
    exit = true;
    if (th.joinable())
    {
        th.join();
    }

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

    if (print_buffer != nullptr)
    {
        free(print_buffer);
        print_buffer = nullptr;
    }
    if (out_buffer != nullptr)
    {
        free(out_buffer);
        out_buffer = nullptr;
    }
}
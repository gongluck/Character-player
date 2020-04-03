#include <iostream>
#include <fstream>

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

const int WIDTH = 80;
const int HEIGHT = 40;
char in_buffer[WIDTH * HEIGHT * 4];
char out_buffer[WIDTH * HEIGHT * 4];
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
    /*static std::ofstream file("save.rgba", std::ios::binary | std::ios::trunc);
    file.write(out_buffer, sizeof(out_buffer));
    static int n = 0;
    if (n++ == 900)
    {
        static std::ofstream filen("saven.rgba", std::ios::binary | std::ios::trunc);
        filen.write(out_buffer, sizeof(out_buffer));
        filen.close();
    }*/

    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0};
    SetConsoleCursorPosition(hConsoleOutput, pos);

    RGBQUAD* rgba = reinterpret_cast<RGBQUAD*>(out_buffer);
    for (int i = 0; i < HEIGHT; ++i)
    {
        for (int j = 0; j < WIDTH; ++j)
        {
            auto point = rgba[WIDTH * i + j];
            auto light = (point.rgbRed + point.rgbGreen + point.rgbBlue) / 3;
            if (light > 127)
            {
                std::cout.put('#');
            }
            else
            {
                std::cout.put(' ');
            }
        }
        std::cout.put('\r');
        std::cout.put('\n');
    }
}

int main()
{
    /*std::ifstream in("saven.rgba", std::ios::binary);
    in.read(in_buffer, sizeof(in_buffer));
    RGBQUAD* rgba = reinterpret_cast<RGBQUAD*>(in_buffer);
    for (int i = 0; i < HEIGHT; ++i)
    {
        for (int j = 0; j < WIDTH; ++j)
        {
            auto point = rgba[WIDTH * i + j];
            auto light = (point.rgbRed + point.rgbGreen + point.rgbBlue) / 3;
            if (light > 127)
            {
                std::cout.put('#');
            }
            else
            {
                std::cout.put(' ');
            }
        }
        std::cout.put('\r');
        std::cout.put('\n');
    }*/

    libvlc_instance_t* inst_ = nullptr;
    libvlc_media_t* media_ = nullptr;
    libvlc_media_player_t* player_ = nullptr;
    int ret = 0;

    libvlc_log_close(nullptr);

    inst_ = libvlc_new(0, nullptr);
    CHECKEQUALRET(inst_, nullptr);

    media_ = libvlc_media_new_path(inst_, "badapple.mp4");
    CHECKEQUALRET(media_, nullptr);

    player_ = libvlc_media_player_new_from_media(media_);
    CHECKEQUALRET(player_, nullptr);

    libvlc_video_set_callbacks(player_, lock, unlock, display, 0);
    libvlc_video_set_format(player_, "RGBA", WIDTH, HEIGHT, WIDTH * 4);
    //libvlc_media_player_set_hwnd(player_, GetDesktopWindow());

    ret = libvlc_media_player_play(player_);
    CHECKNEQUALRET(ret, 0);

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
    if (inst_ != nullptr)
    {
        libvlc_release(inst_);
        inst_ = nullptr;
    }
}
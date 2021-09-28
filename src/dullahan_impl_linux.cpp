#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <string>
#include <fstream>
#include <dirent.h>

namespace
{
std::string getExeCwd()
{
    char path[ 4096 ];
    int len = readlink("/proc/self/exe", path, sizeof(path));
    if (len != -1)
    {
        path[len] = 0;
        return dirname(path) ;
    }
    return "";
}
}

class dullahan_platform_impl_linux : public dullahan_platform_impl
{
    void init() override {}
    void initWidevine(std::string) override {}
    bool useAudioOOP() override { return false; }
    bool useWavAudio() override { return true; }
    bool setVolume(float aVolume) override { return true; }
    void addCommandLines(CefRefPtr<CefCommandLine> command_line) override
    {
        command_line->AppendSwitchWithValue("use-gl","desktop");
    }
};


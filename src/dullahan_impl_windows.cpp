

class dullahan_platform_windows : public dullahan_platform_impl
{
    volatile unsigned long *mSHMVolume;
    HANDLE mFile;
    uint8_t *mMappedFile;
public:
    dullahan_platform_windows()
    {
        mSHMVolume = nullptr;
        mFile = nullptr;
        mMappedFile = nullptr;
    }

    ~dullahan_platform_windows()
    {
        if (mMappedFile)
            ::UnmapViewOfFile(mMappedFile);
        if (mFile)
            ::CloseHandle(mFile);
    }

    void init() override
    {
        std::stringstream strm;

        strm << R"(Local\dullahan_volume.)" << ::GetCurrentProcessId();

        mFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 64, strm.str().c_str());

        if (mFile)
            mMappedFile = (uint8_t*)MapViewOfFile(mFile, FILE_MAP_ALL_ACCESS, 0, 0, 64);

        if (mMappedFile)
        {
            uintptr_t pAligned = reinterpret_cast<uintptr_t>(mMappedFile);
            pAligned += 0xF;
            pAligned &= ~0xF;
            mSHMVolume = (volatile unsigned long*)pAligned;
            ::InterlockedExchange(mSHMVolume, 100L);
        }
    }

    void setVolume(float aVolume) override
    {
        unsigned long volume = static_cast< unsigned long>(100.f * aVolume);
        if (volume < 0)
            volume = 0;
        else if (volume > 100)
            volume = 100;
        if (mSHMVolume)
            ::InterlockedExchange(mSHMVolume, volume);
    }

    void initWidevine(std::string) override
    {
    }

    bool useAudioOOP() override
    {
        return true;
    }

    bool useWavAudio() override
    {
        return true;
    }
   
    void addCommandLines(CefRefPtr<CefCommandLine> command_line)  override
    {
    }

};
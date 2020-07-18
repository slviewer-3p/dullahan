#include <mmdeviceapi.h>
#include <audiopolicy.h>


class dullahan_platform_windows : public dullahan_platform_impl
{
    volatile unsigned long *mPID{ nullptr };
    HANDLE mFile{ nullptr };
    uint8_t *mMappedFile{ nullptr };
    ISimpleAudioVolume *mVolumeControl{ nullptr };
public:
    dullahan_platform_windows()
    {
    }

    ~dullahan_platform_windows()
    {
        if (mMappedFile)
            ::UnmapViewOfFile(mMappedFile);
        if (mFile)
            ::CloseHandle(mFile);

        if (mVolumeControl)
            mVolumeControl->Release();
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
            mPID = (volatile unsigned long*)pAligned;
            ::InterlockedExchange(mPID, 0L);
        }

        ::CoInitialize(nullptr);
    }

    ISimpleAudioVolume *getAudioSession()
    {
        if (mVolumeControl)
            return mVolumeControl;

        if (!mPID)
            return nullptr;

        unsigned long nPID = ::InterlockedExchangeSubtract(mPID, 0);
        if (nPID == 0)
            return nullptr;


        IMMDeviceEnumerator *pDevEnumerator = nullptr;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&pDevEnumerator);

        if (FAILED(hr))
            return nullptr;

        IMMDevice *pDevice = nullptr;

        hr = pDevEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        pDevEnumerator->Release();

        if (FAILED(hr))
            return nullptr;

        IAudioSessionManager2 *pManager = nullptr;
        hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, NULL, (void**)&pManager);
        pDevice->Release();

        if (FAILED(hr))
            return nullptr;

        IAudioSessionEnumerator *pEnum{ nullptr };
        pManager->GetSessionEnumerator(&pEnum);
        pManager->Release();

        if (FAILED(hr))
            return nullptr;
        
        int nSessionCount;
        hr = pEnum->GetCount(&nSessionCount);
        if (FAILED(hr))
            nSessionCount = 0; // Zero it out, that makes the loop below not do anything

        ISimpleAudioVolume *pVolControl{ nullptr };
        for (int nSession = 0; pVolControl == nullptr && nSession < nSessionCount; ++nSession)
        {
            IAudioSessionControl *pSession{ nullptr };
            if (FAILED(pEnum->GetSession(nSession, &pSession)))
                continue;

            IAudioSessionControl2 *pSession2;
            hr = pSession->QueryInterface( __uuidof(IAudioSessionControl2), (void**)&pSession2);
            pSession->Release();

            if (FAILED(hr))
                continue;

            DWORD dwPID{ 0 };
            if (SUCCEEDED(pSession2->GetProcessId(&dwPID)) && dwPID == nPID )
            {
                hr = pSession2->QueryInterface( __uuidof( ISimpleAudioVolume ), (void**)&pVolControl);
                if (FAILED(hr))
                    pVolControl = nullptr; // Should be nullptr already, just be sure.
            }

            pSession2->Release();
        }

        pEnum->Release();
        mVolumeControl = pVolControl;
        return mVolumeControl;    }

    void setVolume(float aVolume) override
    {
        ISimpleAudioVolume *pVol{ getAudioSession() };
        if (!pVol)
            return;
        if (aVolume < 0)
            aVolume = 0.f;
        else if (aVolume > 1.f)
            aVolume = 1.f;

        pVol->SetMasterVolume(aVolume, nullptr);
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
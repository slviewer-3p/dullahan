#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <tlhelp32.h>

std::vector<uint32_t> GetProcessesInGroup()
{
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 ProcessEntry = {};
    std::vector< uint32_t > vChildren{};
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

    DWORD CurrentProcessId = GetCurrentProcessId();
    vChildren.push_back(CurrentProcessId);

    if (Process32First(Snapshot, &ProcessEntry))
    {
        do
        {
            if (ProcessEntry.th32ParentProcessID == CurrentProcessId)
                vChildren.push_back(ProcessEntry.th32ProcessID);
        } while (Process32Next(Snapshot, &ProcessEntry));
    }

    CloseHandle(Snapshot);

    return vChildren;
}


class dullahan_platform_windows : public dullahan_platform_impl
{
    ISimpleAudioVolume *mVolumeControl{ nullptr };
public:
    dullahan_platform_windows()
    {
    }

    ~dullahan_platform_windows()
    {
        if (mVolumeControl)
            mVolumeControl->Release();
    }

    void init() override
    {
        ::CoInitialize(nullptr);
    }

    ISimpleAudioVolume *getAudioSession()
    {
        if (mVolumeControl)
            return mVolumeControl;

        std::vector< uint32_t > vPIDs{ GetProcessesInGroup() };

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
        hr = pManager->GetSessionEnumerator(&pEnum);
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
 
            hr = pEnum->GetSession(nSession, &pSession);

            if (FAILED(hr))
                continue;

            IAudioSessionControl2 *pSession2;
            hr = pSession->QueryInterface( __uuidof(IAudioSessionControl2), (void**)&pSession2);
            pSession->Release();

            if (FAILED(hr))
                continue;

            DWORD dwPID{ 0 };
            hr = pSession2->GetProcessId(&dwPID);
            if (SUCCEEDED(hr) && std::find(vPIDs.begin(), vPIDs.end(), dwPID) != vPIDs.end())
            {
                hr = pSession2->QueryInterface( __uuidof( ISimpleAudioVolume ), (void**)&pVolControl);
                if (FAILED(hr))
                    pVolControl = nullptr; // Should be nullptr already, just be sure.
            }

            pSession2->Release();
        }

        pEnum->Release();
        mVolumeControl = pVolControl;
        return mVolumeControl;
    }

    bool setVolume(float aVolume) override
    {
        ISimpleAudioVolume *pVol{ getAudioSession() };
        if (!pVol)
            return false;
        if (aVolume < 0)
            aVolume = 0.f;
        else if (aVolume > 1.f)
            aVolume = 1.f;

        return SUCCEEDED( pVol->SetMasterVolume(aVolume, nullptr) );
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
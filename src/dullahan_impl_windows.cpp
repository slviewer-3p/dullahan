
volatile unsigned long *shmVolume;

void dullahan_impl::platformInit()
{
	uint32_t parent = ::GetCurrentProcessId();
	std::stringstream strm;

	strm << R"(Local\dullahan_volume.)" << parent;

	HANDLE hFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,	64, strm.str().c_str() );

	uint8_t *pBuff = nullptr;
	if ( hFile )
		pBuff = (uint8_t*)MapViewOfFile(hFile, FILE_MAP_ALL_ACCESS, 0, 0, 64);

	if (pBuff == NULL && hFile)
	{
		CloseHandle(hFile);
	}

	if (pBuff)
	{
		uintptr_t pAligned = reinterpret_cast<uintptr_t>(pBuff);
		pAligned += 0xF;
		pAligned &= ~0xF;
		shmVolume = (volatile unsigned long*)pAligned;
		::InterlockedExchange(shmVolume, 100L);
	}
}

void dullahan_impl::platformSetVolume(float aVolume)
{
	unsigned long volume = 100.f* aVolume;
	if (volume < 0)
		volume = 0;
	else if (volume > 100)
		volume = 100;
	if ( shmVolume)
		::InterlockedExchange(shmVolume, volume);
}

void dullahan_impl::platormInitWidevine(std::string cachePath)
{
}

void dullahan_impl::platformAddCommandLines(CefRefPtr<CefCommandLine> command_line)
{
    if (mForceWaveAudio == true)
    {
        // Grouping these together since they're interconnected.
        // The pair, force use of WAV based audio and the second stops
        // CEF using out of process audio which breaks ::waveOutSetVolume()
        // that ise used to control the volume of media in a web page
        command_line->AppendSwitch("force-wave-audio");

        // <ND> This breaks twitch and friends. Allow to not add this via env override (for debugging)
        char const *pEnv{ getenv("nd_AudioServiceOutOfProcess") };
        bool bDisableAudioServiceOutOfProcess{ true };
        if (pEnv && pEnv[0] == '1')
            bDisableAudioServiceOutOfProcess = false;
    }
}

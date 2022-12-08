#pragma once

#ifdef EXPORT_SMTC_INTEROP
#define SMTC_MANAGER_API __declspec(dllexport)
#else
#define SMTC_MANAGER_API __declspec(dllimport)
#endif

#include <string>

namespace SMTCManager
{
	SMTC_MANAGER_API bool Initialize();
	SMTC_MANAGER_API bool HasSession();

	SMTC_MANAGER_API std::string GetMediaType();
	SMTC_MANAGER_API std::string GetArtist();
	SMTC_MANAGER_API std::string GetTitle();
	SMTC_MANAGER_API std::string GetSubtitle();
	SMTC_MANAGER_API bool HasThumbnail();

	SMTC_MANAGER_API void SkipToNext();
	SMTC_MANAGER_API void SkipToPrevious();
	SMTC_MANAGER_API void Play();
	SMTC_MANAGER_API void Pause();
	SMTC_MANAGER_API void TogglePausePlay();

	SMTC_MANAGER_API void Log(std::string);
};
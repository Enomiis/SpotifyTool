#pragma once

#include <string>

namespace SMTCManager
{
	void test();

	bool Initialize();
	bool HasSession();

	std::string GetMediaType();
	std::string GetArtist();
	std::string GetTitle();
	std::string GetSubtitle();
	bool HasThumbnail();

	void SkipToNext();
	void SkipToPrevious();
	void Play();
	void Pause();
	void TogglePausePlay();

	void Log(std::string);
};
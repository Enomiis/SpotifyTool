#include "pch.h"

#define EXPORT_SMTC_INTEROP
#include "SMTCInterop.h"

#include <iostream>
#include <sstream>
#include <memory>

#include <msclr\marshal_cppstd.h>

using msclr::interop::marshal_as;

ref class Global
{
public:
	static SMTCManager::SMTCManager manager;
};


SMTC_MANAGER_API bool SMTCManager::Initialize()
{
	return Global::manager.Init();
}


SMTC_MANAGER_API bool SMTCManager::HasSession() { return Global::manager.HasSession; }

SMTC_MANAGER_API std::string SMTCManager::GetMediaType() { return marshal_as<std::string>(Global::manager.MediaType); }

SMTC_MANAGER_API std::string SMTCManager::GetArtist() { return marshal_as<std::string>(Global::manager.Artist); }

SMTC_MANAGER_API std::string SMTCManager::GetTitle() { return marshal_as<std::string>(Global::manager.Title); }

SMTC_MANAGER_API std::string SMTCManager::GetSubtitle() { return marshal_as<std::string>(Global::manager.Subtitle); }

SMTC_MANAGER_API bool SMTCManager::HasThumbnail() { return Global::manager.HasThumbnail; }


SMTC_MANAGER_API void SMTCManager::SkipToNext() { Global::manager.SkipToNext(); }

SMTC_MANAGER_API void SMTCManager::SkipToPrevious() { Global::manager.SkipToPrevious(); }

SMTC_MANAGER_API void SMTCManager::Play() { Global::manager.Play(); }

SMTC_MANAGER_API void SMTCManager::Pause() { Global::manager.Pause(); }

SMTC_MANAGER_API void SMTCManager::TogglePausePlay() { Global::manager.TogglePausePlay(); }


SMTC_MANAGER_API void SMTCManager::Log(std::string text)
{
	if (Global::manager.Initialized) Global::manager.Log(marshal_as<System::String^>(text));
}
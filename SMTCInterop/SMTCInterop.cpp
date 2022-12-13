#include "pch.h"

#using "SMTCManager.dll"
#include "SMTCInterop.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <msclr\marshal_cppstd.h>

using msclr::interop::marshal_as;

ref class Global
{
public:
	static SMTCManager::SMTCManager ^manager = SMTCManager::SMTCManager::GetInstance();
};

void SMTCManager::test()
{
	
}

bool SMTCManager::Initialize()
{
	return Global::manager->Init();
}


bool SMTCManager::HasSession() { return Global::manager->HasSession; }

std::string SMTCManager::GetMediaType() { return marshal_as<std::string>(Global::manager->MediaType); }

std::string SMTCManager::GetArtist() { return marshal_as<std::string>(Global::manager->Artist); }

std::string SMTCManager::GetTitle() { return marshal_as<std::string>(Global::manager->Title); }

std::string SMTCManager::GetSubtitle() { return marshal_as<std::string>(Global::manager->Subtitle); }

bool SMTCManager::HasThumbnail() { return Global::manager->HasThumbnail; }


void SMTCManager::SkipToNext() { Global::manager->SkipToNext(); }

void SMTCManager::SkipToPrevious() { Global::manager->SkipToPrevious(); }

void SMTCManager::Play() { Global::manager->Play(); }

void SMTCManager::Pause() { Global::manager->Pause(); }

void SMTCManager::TogglePausePlay() { Global::manager->TogglePausePlay(); }


void SMTCManager::Log(std::string text)
{
	if (Global::manager->Initialized) Global::manager->Log(marshal_as<System::String^>(text));
}
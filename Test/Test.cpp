// Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SMTCInterop.h>

int main()
{
    if (SMTCManager::Initialize())
    {
        std::cout << SMTCManager::GetArtist() << std::endl;
        std::cout << SMTCManager::GetTitle() << std::endl;
        std::cout << SMTCManager::GetSubtitle() << std::endl;
        SMTCManager::TogglePausePlay();
    }
    else
    {
        std::cout << "Couldn't open SMTC Session Manager" << std::endl;
    }
}


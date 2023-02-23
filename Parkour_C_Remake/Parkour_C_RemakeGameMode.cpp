// Copyright Epic Games, Inc. All Rights Reserved.

#include "Parkour_C_RemakeGameMode.h"
#include "Parkour_C_RemakeCharacter.h"
#include "UObject/ConstructorHelpers.h"

AParkour_C_RemakeGameMode::AParkour_C_RemakeGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

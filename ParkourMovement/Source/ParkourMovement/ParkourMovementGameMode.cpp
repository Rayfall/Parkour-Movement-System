// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParkourMovementGameMode.h"
#include "ParkourMovementCharacter.h"
#include "UObject/ConstructorHelpers.h"

AParkourMovementGameMode::AParkourMovementGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

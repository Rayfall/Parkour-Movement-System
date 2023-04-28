// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Math/Color.h"
#include "Engine/World.h"
#include "ParkourMovement/ParkourMovement.h"

/************************************************************/
/*------------------ Initial Set Up ------------------------*/
/************************************************************/

// Sets default values for this component's properties
UParkourMovementComponent::UParkourMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
	// Initialize Character, CharacterMovement Component, and the Previous and Current Parkour modes

	PreviousParkourMode = EParkourMovement::None;
	CurrentParkourMode = EParkourMovement::None;

	// Main Event Delegates
	this->OnMovementChanged.AddDynamic(this, &UParkourMovementComponent::MovementChanged);
	this->OnParkourChanged.AddDynamic(this, &UParkourMovementComponent::ParkourMovementChanged);
	this->OnJumpEvent.AddDynamic(this, &UParkourMovementComponent::Jump);
	//this->OnCameraShakeEvent.AddDynamic(this, &UParkourMovementComponent::PlayCameraShake);
	this->OnLandEvent.AddDynamic(this, &UParkourMovementComponent::Land);
	this->OnSprintEvent.AddDynamic(this, &UParkourMovementComponent::Sprint);
	this->OnQueuesCheckEvent.AddDynamic(this, &UParkourMovementComponent::CheckQueues);
	this->OnCrouchSlideEvent.AddDynamic(this, &UParkourMovementComponent::CrouchSlide);


	// In Program Events
	this->OnWallRunEnd.AddDynamic(this, &UParkourMovementComponent::WallRunEnd);
	this->OnUpdateEvent.AddDynamic(this, &UParkourMovementComponent::UpdateSequence);

	/*
	// JumpLand Camera Shake Set Up, All Oscillation 0.25,0.1,0.2, RotOscillation Pitch Amp -50, Freq 1, Initial OffsetZero
	JumpLandCamera.OscillationDuration = 0.25f;
	JumpLandCamera.OscillationBlendInTime = 0.1f;
	JumpLandCamera.OscillationBlendOutTime = 0.2f;

	JumpLandCamera.RotOscillation.Pitch.Amplitude = -50.f;
	JumpLandCamera.RotOscillation.Pitch.Frequency = 1.f;
	JumpLandCamera.RotOscillation.Pitch.InitialOffset;

	// LedgeGrab Camera Shake Set Up, All Oscillation 0.5,0.1,0.2, RotOscillation Pitch -100,1,Zero, LocOscillation Z 100,1,Zero
	LedgeGrabCamera;

	// Quick Mantle Camera Shake Set Up, All Oscillation 0.25,0.1,0.2, RotOscillation Roll -25, LocOscillation Z 100,1,Zero
	MantleCamera;

	// Mantle Camera Shake Set Up, All Oscillation 0.5,0.1,0.2, RotOscillation Pitch -100,1,Zero, LocOscillation Z 100,1,Zero
	QuickMantleCamera;
	*/
}

// Called when the game starts
void UParkourMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

// Called every frame
void UParkourMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

/************************************************************/
/*------------------------ Main ----------------------------*/
/************************************************************/

void UParkourMovementComponent::Land()
{
	EndEvents();
	CloseGates();
	// Broadcast Land Camera Shake
	//OnCameraShakeEvent.Broadcast(JumpLandCamera);
}

void UParkourMovementComponent::Jump()
{
	// If No Current Parkour Mode is used, check if the character is falling.
	if (CurrentParkourMode == EParkourMovement::None) {
		// If Not falling, Call OpenGates and then the PlayCameraShake Event
		if (!CharacterMovementComponent->IsFalling()) {
			OpenGates();
			// Broadcast Jump Camera Shake
			//OnCameraShakeEvent.Broadcast();
		}
	}
	else {
		// If in a Current Parkour Mode, go through the sequence of Jump Events.
		JumpEvent();
	}
}

void UParkourMovementComponent::PlayCameraShake(TSubclassOf<UCameraShakeBase> Camera)
{

	if (CameraShake) {
		// Play World Camera Shake with the Shake Camera Selected, Epicenter of Character Location
		// Inner Radius of 0, Outer Radius of 100, Falloff at 1, and No Orient Shake Towards Epicenter
		UGameplayStatics::PlayWorldCameraShake(this, Camera, Character->GetActorLocation(), 0.f, 100.f, 1.0f, false);
	}
}

void UParkourMovementComponent::CrouchSlide()
{
	if (CancelMovement()) {
		FString message = "Parkour Movement Component_CrouchSlide: Cancel Movement Returned True.";
		UE_LOG(LogTemp, Warning, TEXT("%s"), *message);
	}
	else {
		if (MacroCanSlide()) {
			if (CharacterMovementComponent->IsWalking()) {
				SlideStart();
			}
			else {
				SlideQueued = true;
			}
		}
		else {
			CrouchToggle();
		}
	}
}

void UParkourMovementComponent::CheckQueues()
{
	if (SlideQueued) {
		SlideStart();
	}
	else if (SprintQueued) {
		SprintStart();
	}
}

void UParkourMovementComponent::MovementChanged(EMovementMode PrevMovementMode, EMovementMode NewMovementMode)
{
	if (IsValid(CharacterMovementComponent)) {
		PreviousMovementMode = PrevMovementMode;
		CurrentMovementMode = NewMovementMode;

		OpenMovementGates();
	}
}

void UParkourMovementComponent::ParkourMovementChanged(EParkourMovement PrevParkourMode, EParkourMovement NewParkourMode)
{
	PreviousParkourMode = PrevParkourMode;
	CurrentParkourMode = NewParkourMode;

	ResetMovement();
}

void UParkourMovementComponent::Sprint()
{
	SprintStart();
}

bool UParkourMovementComponent::SetParkourMovementMode(EParkourMovement NewMode)
{
	bool results = false;

	if (NewMode == CurrentParkourMode) {

	}
	else {
		// Call Event Dispatcher to change Current Parkour Mode to New Parkour Mode
		OnParkourChanged.Broadcast(CurrentParkourMode, NewMode);
		results = true;
	}
	return results;
}

void UParkourMovementComponent::ResetMovement()
{
	//if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Reset Movement Called")); }

	if ((CurrentParkourMode == EParkourMovement::None) || (CurrentParkourMode == EParkourMovement::Crouch)) {
		CharacterMovementComponent->bOrientRotationToMovement = true;
		Character->bUseControllerRotationYaw = DefaultUseControllerRotationYaw;
		CharacterMovementComponent->GravityScale = DefaultGravity;
		CharacterMovementComponent->GroundFriction = DefaultGroundFriction;
		CharacterMovementComponent->BrakingDecelerationWalking = DefaultBrakingDeceleration;
		CharacterMovementComponent->MaxWalkSpeed = DefaultMaxWalkSpeed;
		CharacterMovementComponent->MaxWalkSpeedCrouched = DefaultMaxCrouchSpeed;
		CharacterMovementComponent->SetPlaneConstraintEnabled(false);

		TEnumAsByte<EMovementMode> NewMode;

		switch (PreviousParkourMode) {
		case EParkourMovement::None: NewMode = EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::LeftWallRun: NewMode = EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::RightWallRun: NewMode = EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::VerticalWallRun: NewMode = EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::LedgeGrab: NewMode = EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::Mantle: NewMode = EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::Slide: NewMode = EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::Crouch: NewMode = EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::Sprint: NewMode = EMovementMode::MOVE_Walking;
			break;
		}
		CharacterMovementComponent->SetMovementMode(NewMode, 0);
	}
	else {
		CharacterMovementComponent->bOrientRotationToMovement = (CurrentParkourMode == EParkourMovement::Sprint);

		Character->bUseControllerRotationYaw = ((CurrentParkourMode == EParkourMovement::Sprint) && DefaultUseControllerRotationYaw);
	}
}

bool UParkourMovementComponent::CancelMovement()
{
	bool Results = true;

	if ((CurrentParkourMode == EParkourMovement::LedgeGrab) || (CurrentParkourMode == EParkourMovement::VerticalWallRun) || (CurrentParkourMode == EParkourMovement::Mantle)) {
		VerticalWallRunEnd(0.5);
	}
	else if (MacroWallRunning()) {
		WallRunEnd(0.5);
	}
	else {
		Results = false;
	}

	return Results;
}

bool UParkourMovementComponent::ForwardTracer(FHitResult& OutResult)
{
	FHitResult HitResult;
	FVector EndVector = (MacroMantleVectorsFeet() + (Character->GetActorForwardVector() * 50));
	FLinearColor ColorOne = FLinearColor(0, 1, 0, 1);
	FLinearColor ColorTwo = FLinearColor(1, 0, 0, 1);

	UKismetSystemLibrary::CapsuleTraceSingle(Character->GetCapsuleComponent(), MacroMantleVectorsFeet(), EndVector, 10, 5, TraceTypeQuery3, false, ActorsToIgnore, EDrawDebugTrace::Type::None, HitResult, true, ColorTwo, ColorOne, 5);

	if ((HitResult.Normal.Z >= -0.1) && HitResult.bBlockingHit) {
		OutResult = HitResult;
		return true;
	}
	return false;
}

void UParkourMovementComponent::UpdateEventMethod()
{
	OnUpdateEvent.Broadcast();
}

// Initialize Event called using Initialize Broadcast
void UParkourMovementComponent::Initialize(ACharacter* PlayerCharacter)
{
	Character = PlayerCharacter;
	CharacterMovementComponent = PlayerCharacter->GetCharacterMovement();

	// Update Camera Properties
	DefaultUseControllerRotationYaw = PlayerCharacter->bUseControllerRotationYaw;

	DefaultGravity = CharacterMovementComponent->GravityScale;
	DefaultGroundFriction = CharacterMovementComponent->GroundFriction;
	DefaultMaxWalkSpeed = CharacterMovementComponent->MaxWalkSpeed;
	DefaultMaxCrouchSpeed = CharacterMovementComponent->MaxWalkSpeedCrouched;
	DefaultBrakingDeceleration = CharacterMovementComponent->BrakingDecelerationWalking;

	GetWorld()->GetTimerManager().SetTimer(UpdateEventHandle, this, &UParkourMovementComponent::UpdateEventMethod, InitializeTime, true);
}

/************************************************************/
/*---------------------- Wall Run --------------------------*/
/************************************************************/

void UParkourMovementComponent::WallRunEnd(float ResetTime)
{
	if (MacroWallRunning()) {
		if (SetParkourMovementMode(EParkourMovement::None)) {
			// Call Close Wall Run Gate Event Dispatcher
			CloseWallRunGate();

			// Call the update to stop the wall run
			//WallRunGate();

			// Open the gate upon finishing.
			GetWorld()->GetTimerManager().SetTimer(WallRunOpenGateEventHandle, this, &UParkourMovementComponent::OpenWallRunGate, ResetTime, false);

			// Clear the Wall Run Enable Gravity Timer
			GetWorld()->GetTimerManager().ClearTimer(WallRunEnableGravityEventHandle);

			// Set Wall Run Gravity to false;
			bIsWallRunGravity = false;
		}
		else {
			FString message = "Parkour Movement Component_WallRunEnd: SetParkourMode to None Failed.";
			//UE_LOG(LogTemp, Warning, TEXT("%s"), *message);
			//return;
		}
	}
	else {
		FString message = "Parkour Movement Component_WallRunEnd: MacroWallRunning returned false.";
		//UE_LOG(LogTemp, Warning, TEXT("%s"), *message);
		//return;
	}
}

bool UParkourMovementComponent::WallRunMovement(FVector Start, FVector End, float WallRunDirection)
{
	// Create a Hit Result for the Outhit Parameters
	FHitResult Hit;

	// Call Line Trace By Channel with the Start and End Vector, Needing the bool BlockingHit, OutHitImpactPoint, and OutHitNormal.
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility)) {
		if (Hit.bBlockingHit)
		{
			// Set WallRunNormal and Location to their respects variables
			WallRunNormal = Hit.Normal;
			WallRunLocation = Hit.ImpactPoint;

			// Call Macro Valid Wall Run Vector and get the charactermovement if falling
			if (MacroValidWallRunVector(Hit.Normal) && Character->GetCharacterMovement()->IsFalling())
			{
				float select = UKismetMathLibrary::SelectFloat(WallRunSprintSpeed, WallRunSpeed, SprintQueued);
				float FResults = select * WallRunDirection;
				FVector VResults = FVector::CrossProduct(WallRunNormal, { 0, 0, 1 });
				bool BResults = (!MacroWallRunning() || !bIsWallRunGravity);

				// Launch character to wall, sticking them in the forward direction
				Character->LaunchCharacter((VResults * FResults), true, BResults);
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

void UParkourMovementComponent::WallRunGravity()
{
	float Results = UKismetMathLibrary::FInterpTo(CharacterMovementComponent->GravityScale, WallRunTargetGravity, GetWorld()->GetDeltaSeconds(), WallRunStartSpeed);
	CharacterMovementComponent->GravityScale = Results;
}

void UParkourMovementComponent::WallRunEnableGravity()
{
	if (MacroWallRunning()) {
		bIsWallRunGravity = true;
	}
	else {
		bIsWallRunGravity = false;
	}
}

void UParkourMovementComponent::CorrectWallRunLocation()
{
	if (MacroWallRunning()) {
		FLatentActionInfo LatentInfo;
		LatentInfo.CallbackTarget = this;

		UKismetSystemLibrary::MoveComponentTo(Character->GetRootComponent(), WallRunTargetVector(), WallRunTargetRotation(), false, false, 0.1, false, EMoveComponentAction::Move, LatentInfo);
	}
}

FVector UParkourMovementComponent::WallRunTargetVector()
{
	float CapRadius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	return (WallRunLocation + (WallRunNormal * CapRadius));
}

FRotator UParkourMovementComponent::WallRunTargetRotation()
{
	float RotX = Character->GetCapsuleComponent()->GetRelativeRotation().Roll;
	float RotY = Character->GetCapsuleComponent()->GetRelativeRotation().Pitch;
	FRotator Conversion = UKismetMathLibrary::Conv_VectorToRotator(WallRunNormal);
	float RawZValue = Conversion.Yaw;
	float Selection = UKismetMathLibrary::SelectFloat(90, -90, (CurrentParkourMode == EParkourMovement::LeftWallRun));

	FRotator Results = UKismetMathLibrary::MakeRotator(RotX, RotY, (RawZValue - Selection));
	return Results;
}

void UParkourMovementComponent::WallRunUpdate()
{
	if (MacroCanWallRun()) {
		// Call Function WallRunMovement With Character's Location Vector, the Wall Run End Right Vector, and Run Direction of -1.0. Returns a Boolean.

		if (WallRunMovement(Character->GetActorLocation(), MacroWallRunEndVectorsRight(), -1.0)) {
			if (SetParkourMovementMode(EParkourMovement::RightWallRun)) {
				// Call Function Set Timer By Function Name, with function name WallRunEnableGravity, and a float time set as 1
				GetWorld()->GetTimerManager().SetTimer(WallRunEnableGravityEventHandle, this, &UParkourMovementComponent::WallRunEnableGravity, 1.0f, false);

				// Call Delegate Correct Wall Run Location
				CorrectWallRunLocation();
				// Call Delegate Wall Run Gravity
				WallRunGravity();
			}
			else {
				// Call Delegate Wall Run Gravity
				WallRunGravity();
			}
		}
		else {
			if (CurrentParkourMode == EParkourMovement::RightWallRun) {
				// Call Wall Run End at 0.5 seconds
				OnWallRunEnd.Broadcast(0.5);
			}
			else {
				if (WallRunMovement(Character->GetActorLocation(), MacroWallRunEndVectorsLeft(), 1.0)) {
					if (SetParkourMovementMode(EParkourMovement::LeftWallRun)) {
						// Call Function Set Timer By Function Name, with function name WallRunEnableGravity, and a float time set as 1
						GetWorld()->GetTimerManager().SetTimer(WallRunEnableGravityEventHandle, this, &UParkourMovementComponent::WallRunEnableGravity, 1.0f, false);

						// Call Delegate Correct Wall Run Location
						CorrectWallRunLocation();

						// Call Delegate Wall Run Gravity
						WallRunGravity();
					}
					else {
						// Call Delegate Wall Run Gravity
						WallRunGravity();
					}
				}
				else {
					// Call Wall Run End at 0.5 seconds
					OnWallRunEnd.Broadcast(0.5);
				}
			}
		}
	}
	else {
		// Call Wall Run End at 1 second
		OnWallRunEnd.Broadcast(1.0);
	}
}

/************************************************************/
/*----------------- Vertical Wall Run ----------------------*/
/************************************************************/

void UParkourMovementComponent::VerticalWallRunUpdate()
{
	if (MacroCanVerticalWallRun()) {
		FHitResult HitResults;

		FLinearColor ColorOne = FLinearColor(0, 1, 0, 1);
		FLinearColor ColorTwo = FLinearColor(1, 0, 0, 0);

		if (UKismetSystemLibrary::CapsuleTraceSingle(
			Character->GetCapsuleComponent(),
			MacroMantleVectorsEyes(),
			MacroMantleVectorsFeet(),
			20,
			10,
			ETraceTypeQuery::TraceTypeQuery4,
			false,
			ActorsToIgnore,
			EDrawDebugTrace::Type::None,
			HitResults,
			true,
			ColorTwo,
			ColorOne,
			5)) {

			MantleTraceDistance = HitResults.Distance;
			LedgeFloorPosition = HitResults.ImpactPoint;
			FHitResult ForwardTraceHitResults;

			if (ForwardTracer(ForwardTraceHitResults) && CharacterMovementComponent->IsWalkable(HitResults)) {
				LedgeClimbWallPosition = ForwardTraceHitResults.ImpactPoint;
				LedgeClimbWallNormal = ForwardTraceHitResults.ImpactNormal;
				MantlePosition = (LedgeFloorPosition + FVector(0, 0, MantleZOffset()));
				CloseVerticalWallRunGate();
				LedgeGrab();

				FVector Start = Character->GetActorLocation();
				FVector End = Character->GetActorLocation() - (Character->GetActorUpVector() * CapsuleZOffset());
				FHitResult LineHitResult;

				bool Results = UKismetSystemLibrary::LineTraceSingle(this, Start, End, ETraceTypeQuery::TraceTypeQuery4, false, ActorsToIgnore, EDrawDebugTrace::Type::None, LineHitResult, true, FLinearColor(1, 0, 0, 1), FLinearColor(0, 1, 0, 1), 5);
				LedgeCloseToGround = Results;

				if (MacroQuickMantle()) {
					OpenMantleCheckGate();
				}
				else {
					CorrectLedgeLocation();
					GetWorld()->GetTimerManager().SetTimer(MantleCheckEventHandle, this, &UParkourMovementComponent::OpenMantleCheckGate, 0.25f, false);
				}
			}
			else {
				VerticalWallRunMovement();
			}
		}
		else {
			VerticalWallRunMovement();
		}
	}
	else {
		VerticalWallRunEnd(0.35);
	}
}

void UParkourMovementComponent::VerticalWallRunEnd(float ResetTime)
{
	if ((CurrentParkourMode == EParkourMovement::LedgeGrab) || (CurrentParkourMode == EParkourMovement::VerticalWallRun) || (CurrentParkourMode == EParkourMovement::Mantle)) {
		if (SetParkourMovementMode(EParkourMovement::None)) {
			// Close The Vertical Wall Run Gate
			CloseVerticalWallRunGate();

			// Close Mantle Check Gate
			CloseMantleCheckGate();

			LedgeCloseToGround = false;

			GetWorld()->GetTimerManager().SetTimer(VerticalRunEndGateEventHandle, this, &UParkourMovementComponent::OpenVerticalWallRunGate, ResetTime, false);
			GetWorld()->GetTimerManager().SetTimer(CheckQueuesEventHandle, this, &UParkourMovementComponent::CheckQueues, 0.02f, false);
		}
	}
}

void UParkourMovementComponent::VerticalWallRunMovement()
{
	FHitResult Hit;
	if (ForwardTracer(Hit)) {
		VerticalWallRunLocation = Hit.Location;
		VerticalWallRunNormal = Hit.Normal;

		if (SetParkourMovementMode(EParkourMovement::VerticalWallRun)) {
			CorrectVerticalWallRunLocation();

			FVector LaunchResults = FVector(VerticalWallRunNormal.X * -600.0f, VerticalWallRunNormal.Y * -600.0f, VerticalWallRunSpeed);
			Character->LaunchCharacter(LaunchResults, true, true);
		}
		else {
			FVector LaunchResults = FVector(VerticalWallRunNormal.X * -600.0f, VerticalWallRunNormal.Y * -600.0f, VerticalWallRunSpeed);
			Character->LaunchCharacter(LaunchResults, true, true);
		}
	}
	else {
		VerticalWallRunEnd(0.35);
	}
}

void UParkourMovementComponent::CorrectVerticalWallRunLocation()
{
	FLatentActionInfo LatentActionInfo;
	LatentActionInfo.CallbackTarget = this;

	if (CurrentParkourMode == EParkourMovement::VerticalWallRun) {
		UKismetSystemLibrary::MoveComponentTo(Character->GetRootComponent(), VerticalWallRunTargetLocation(), VerticalWallRunTargetRotation(), false, false, 0.1, false, EMoveComponentAction::Move, LatentActionInfo);
	}
}

FVector UParkourMovementComponent::VerticalWallRunTargetLocation()
{
	FVector Location = VerticalWallRunNormal * (Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius());

	return (VerticalWallRunLocation + Location);
}

FRotator UParkourMovementComponent::VerticalWallRunTargetRotation()
{
	FRotator RawYaw = UKismetMathLibrary::Conv_VectorToRotator(VerticalWallRunNormal);
	float Yaw = RawYaw.Yaw;
	FRotator results = UKismetMathLibrary::MakeRotator(Character->GetCapsuleComponent()->GetRelativeRotation().Roll, Character->GetCapsuleComponent()->GetRelativeRotation().Pitch, (Yaw - 180.0f));
	return results;
}

void UParkourMovementComponent::CorrectLedgeLocation()
{
	FLatentActionInfo LatentActionInfo;
	LatentActionInfo.CallbackTarget = this;

	if (CurrentParkourMode == EParkourMovement::LedgeGrab) {
		UKismetSystemLibrary::MoveComponentTo(Character->GetRootComponent(), LedgeTargetLocation(), LedgeTargetRotation(), false, false, 0.1, false, EMoveComponentAction::Move, LatentActionInfo);
	}
}

FVector UParkourMovementComponent::LedgeTargetLocation()
{
	FVector RollPitchResults = (LedgeClimbWallPosition + (LedgeClimbWallNormal * Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius()));
	float YawResults = LedgeFloorPosition.Z - Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	FVector results = UKismetMathLibrary::MakeVector(RollPitchResults.X, RollPitchResults.Y, YawResults);
	return results;
}

FRotator UParkourMovementComponent::LedgeTargetRotation()
{
	FRotator YawResults = UKismetMathLibrary::Conv_VectorToRotator(LedgeClimbWallNormal);
	float Yaw = YawResults.Yaw;
	FRotator results = UKismetMathLibrary::MakeRotator(Character->GetCapsuleComponent()->GetRelativeRotation().Roll, Character->GetCapsuleComponent()->GetRelativeRotation().Pitch, (Yaw - 180.0f));
	return results;
}

float UParkourMovementComponent::MantleZOffset()
{
	return Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * 1;
}

float UParkourMovementComponent::CapsuleZOffset()
{
	return Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 40.f;
}

void UParkourMovementComponent::VerticalWallRunEndEvent()
{
	GetWorld()->GetTimerManager().ClearTimer(VerticalWallRunEndEventHandle);

	if (CurrentParkourMode == EParkourMovement::VerticalWallRun) {
		VerticalWallRunEnd(2);
	}
}

/************************************************************/
/*------------------------ Jump ----------------------------*/
/************************************************************/
void UParkourMovementComponent::WallRunJump()
{
	if (MacroWallRunning()) {
		WallRunEnd(0.35);

		float LaunchX = (WallRunJumpOffForce * WallRunNormal.X);
		float LaunchY = (WallRunJumpOffForce * WallRunNormal.Y);
		FVector Launch = { LaunchX, LaunchY, WallRunJumpHeight };

		Character->LaunchCharacter(Launch, false, true);
	}
}

void UParkourMovementComponent::LedgeGrabJump()
{
	if ((CurrentParkourMode == EParkourMovement::LedgeGrab) || (CurrentParkourMode == EParkourMovement::VerticalWallRun) || (CurrentParkourMode == EParkourMovement::Mantle)) {
		VerticalWallRunEnd(0.35);

		float LaunchX = (LedgeGrabJumpOffForce * VerticalWallRunNormal.X);
		float LaunchY = (LedgeGrabJumpOffForce * VerticalWallRunNormal.Y);
		FVector Launch = { LaunchX, LaunchY, LedgeGrabJumpHeight };

		Character->LaunchCharacter(Launch, true, true);
	}
}

void UParkourMovementComponent::SlideJump()
{
	if (CurrentParkourMode == EParkourMovement::Slide) {
		SlideEnd(false);
	}
}

void UParkourMovementComponent::CrouchJump()
{
	if (CurrentParkourMode == EParkourMovement::Crouch) {
		CrouchEnd();
	}
}

void UParkourMovementComponent::SprintJump()
{
	if (CurrentParkourMode == EParkourMovement::Sprint) {
		SprintEnd();
		SprintQueued = true;
	}
}

/************************************************************/
/*--------------------- Ledge Grab -------------------------*/
/************************************************************/
void UParkourMovementComponent::LedgeGrab()
{
	if (SetParkourMovementMode(EParkourMovement::LedgeGrab)) {
		CharacterMovementComponent->DisableMovement();
		CharacterMovementComponent->StopMovementImmediately();
		CharacterMovementComponent->GravityScale = 0;

		// Broadcast Ledge Grab Camera Shake
		//OnCameraShakeEvent.Broadcast();
	}
}

/************************************************************/
/*----------------------- Sprint ---------------------------*/
/************************************************************/

void UParkourMovementComponent::SprintUpdate()
{
	if (CurrentParkourMode == EParkourMovement::Sprint) {
		if (!(MacroForwardInput() > 0)) {
			SprintEnd();
		}
	}
}

void UParkourMovementComponent::SprintEnd()
{
	if (CurrentParkourMode == EParkourMovement::Sprint) {
		if (SetParkourMovementMode(EParkourMovement::None)) {
			CloseSprintGate();
			GetWorld()->GetTimerManager().SetTimer(OpenSprintGateEventHandle, this, &UParkourMovementComponent::OpenSprintGate, 0.1f, false);
		}
	}
}

void UParkourMovementComponent::SprintStart()
{
	SlideEnd(false);
	CrouchEnd();

	if (MacroCanSprint()) {
		if (SetParkourMovementMode(EParkourMovement::Sprint)) {
			CharacterMovementComponent->MaxWalkSpeed = SprintSpeed;
			OpenSprintGate();
			SprintQueued = false;
			SlideQueued = false;
		}
	}
}

/************************************************************/
/*----------------------- Crouch ---------------------------*/
/************************************************************/

void UParkourMovementComponent::CrouchStart()
{
	if (CurrentParkourMode == EParkourMovement::None) {
		Character->Crouch();
		SetParkourMovementMode(EParkourMovement::Crouch);
		SprintQueued = false;
		SlideQueued = false;
	}
}

void UParkourMovementComponent::CrouchEnd()
{
	if (CurrentParkourMode == EParkourMovement::Crouch) {
		Character->UnCrouch();
		SetParkourMovementMode(EParkourMovement::None);
		SprintQueued = false;
		SlideQueued = false;
	}
}

void UParkourMovementComponent::CrouchToggle()
{
	switch (CurrentParkourMode) {
	case EParkourMovement::None: CrouchStart();
		break;
	case EParkourMovement::Crouch: CrouchEnd();
		break;
	default: UE_LOG(LogTemp, Warning, TEXT("None"));
		break;
	}
}

/************************************************************/
/*------------------------ Slide ---------------------------*/
/************************************************************/

void UParkourMovementComponent::SlideUpdate()
{
	if (CurrentParkourMode == EParkourMovement::Slide) {
		if (CharacterMovementComponent->Velocity.Length() <= 35.f) {
			SlideEnd(true);
		}
	}
}

void UParkourMovementComponent::SlideStart()
{
	if (MacroCanSlide() && CharacterMovementComponent->IsWalking()) {
		SprintEnd();

		SetParkourMovementMode(EParkourMovement::Slide);
		Character->Crouch();

		CharacterMovementComponent->GroundFriction = 0;
		CharacterMovementComponent->BrakingDecelerationWalking = 1400;
		CharacterMovementComponent->MaxWalkSpeedCrouched = 0;

		CharacterMovementComponent->SetPlaneConstraintFromVectors(VelocityNormal(), Character->GetActorUpVector());

		CharacterMovementComponent->SetPlaneConstraintEnabled(true);

		if (GetSlideVector().Z <= 0.02) {
			CharacterMovementComponent->AddImpulse((GetSlideVector() * SlideImpulseAmount), true);
			OpenSlideGate();
			SprintQueued = false;
			SlideQueued = false;
		}
		else {
			OpenSlideGate();
			SprintQueued = false;
			SlideQueued = false;
		}
	}
}

void UParkourMovementComponent::SlideEnd(bool IsCrouched)
{
	EParkourMovement NewMode = EParkourMovement::None;
	if (CurrentParkourMode == EParkourMovement::Slide) {
		switch (IsCrouched) {
		case false: NewMode = EParkourMovement::None;
			break;
		case true: NewMode = EParkourMovement::Crouch;
			break;
		}

		if (SetParkourMovementMode(NewMode)) {
			CloseSlideGate();

			if (!IsCrouched) {
				Character->UnCrouch();
			}
		}
	}
}

FVector UParkourMovementComponent::VelocityNormal()
{
	return UKismetMathLibrary::Normal(CharacterMovementComponent->Velocity, 0.0001);
}

FVector UParkourMovementComponent::GetSlideVector()
{
	FHitResult HitResults;

	FLinearColor ColorOne = FLinearColor(0, 1, 0, 1);
	FLinearColor ColorTwo = FLinearColor(1, 0, 0, 0);

	FVector Start = Character->GetActorLocation();
	FVector End = (Character->GetActorLocation() + (Character->GetActorUpVector() * -200.f));

	UKismetSystemLibrary::LineTraceSingle(this, Start, End, TraceTypeQuery5, false, ActorsToIgnore, EDrawDebugTrace::Type::None, HitResults, true, ColorOne, ColorTwo, 5);

	FVector CrossResults = UKismetMathLibrary::Cross_VectorVector(HitResults.ImpactNormal, Character->GetActorRightVector());
	//GetWorld()->LineTraceSingleByChannel(HitResults, Start, End, ECC_Visibility); /*Duh, will fix all the kismets after this*/
	FVector Results = CrossResults * -1.f;
	return Results;
}

/************************************************************/
/*----------------------- Mantle ---------------------------*/
/************************************************************/
void UParkourMovementComponent::MantleMovement()
{
	FRotator CurrentRotator = Character->GetController()->GetControlRotation();
	FRotator TargetRotator = UKismetMathLibrary::FindLookAtRotation(FVector(Character->GetActorLocation().X, Character->GetActorLocation().Y, 0), FVector(MantlePosition.X, MantlePosition.Y, 0));
	FRotator InterpR = UKismetMathLibrary::RInterpTo(CurrentRotator, TargetRotator, GetWorld()->DeltaTimeSeconds, 7.0f);
	Character->GetController()->SetControlRotation(InterpR);

	FVector CurrentVector = Character->GetActorLocation();
	FVector InterpV = UKismetMathLibrary::VInterpTo(CurrentVector, MantlePosition, GetWorld()->DeltaTimeSeconds, UKismetMathLibrary::SelectFloat(QuickMantleSpeed, MantleSpeed, MacroQuickMantle()));
	Character->SetActorLocation(InterpV);


	float Distance = UKismetMathLibrary::Vector_Distance(Character->GetActorLocation(), MantlePosition);
	if (Distance < 8) {
		VerticalWallRunEnd(0.5);
	}
}

/************************************************************/
/*------------------- Camera Properties --------------------*/
/************************************************************/
void UParkourMovementComponent::UpdateCameraProperties()
{
	DefaultUseControllerRotationYaw = Character->bUseControllerRotationYaw;
}

void UParkourMovementComponent::CameraTilt(float TargetXRoll)
{
	// Create the starting rotation for the camera tilt
	FRotator Starter = FRotator(TargetXRoll, Character->GetController()->GetControlRotation().Pitch, Character->GetController()->GetControlRotation().Yaw);

	// Create the new rotation from RInterpTo
	FRotator NewRotation = UKismetMathLibrary::RInterpTo(Character->GetController()->GetControlRotation(), Starter, GetWorld()->GetDeltaSeconds(), 10.f);

	// Set the Character's control rotation to the new rotation
	Character->GetController()->SetControlRotation(NewRotation);
}

void UParkourMovementComponent::CameraTick()
{
	//UE_LOG(LogTemp, Warning, TEXT("Current Parkour Mode: %s"), *UEnum::GetValueAsString(CurrentParkourMode));

	/*
	switch (CurrentParkourMode) {
	case EParkourMovement::LeftWallRun: CameraTilt(15.f); UE_LOG(LogTemp, Warning, TEXT("Camera Tilting Left"));
		break;
	case EParkourMovement::RightWallRun: CameraTilt(-15.f); UE_LOG(LogTemp, Warning, TEXT("Camera Tilting Right"));
		break;
	case EParkourMovement::None:CameraTilt(0.f);
		break;
	case EParkourMovement::VerticalWallRun:CameraTilt(0.f);
		break;
	case EParkourMovement::LedgeGrab:CameraTilt(0.f);
		break;
	case EParkourMovement::Mantle:CameraTilt(0.f);
		break;
	case EParkourMovement::Slide:CameraTilt(0.f);
		break;
	case EParkourMovement::Crouch:CameraTilt(0.f);
		break;
	case EParkourMovement::Sprint: CameraTilt(0.f);
		break;
	}*/
}

/************************************************************/
/*------------------------ Events --------------------------*/
/************************************************************/
void UParkourMovementComponent::EndEvents()
{
	WallRunEnd(0);
	VerticalWallRunEnd(0);
	SprintEnd();
	SlideEnd(false);
}

void UParkourMovementComponent::JumpEvent()
{
	WallRunJump();
	LedgeGrabJump();
	SlideJump();
	CrouchJump();
	SprintJump();
}

/************************************************************/
/*------------------------ Gates ---------------------------*/
/************************************************************/
void UParkourMovementComponent::OpenGates()
{
	OpenWallRunGate();
	OpenVerticalWallRunGate();
	OpenSlideGate();
	OpenSprintGate();
}

void UParkourMovementComponent::CloseGates()
{
	CloseWallRunGate();
	CloseVerticalWallRunGate();
	CloseSlideGate();
	CloseSprintGate();
}

void UParkourMovementComponent::OpenMovementGates()
{
	if ((PreviousMovementMode == EMovementMode::MOVE_Walking) && (CurrentMovementMode == EMovementMode::MOVE_Falling))
	{
		SprintJump();
		EndEvents();
		OpenGates();
	}
	else if ((PreviousMovementMode == EMovementMode::MOVE_Falling) && (CurrentMovementMode == EMovementMode::MOVE_Walking)) {
		CheckQueues();
	}
}

void UParkourMovementComponent::WallRunGate()
{
	if (IsWallrunGateOpen) {
		WallRunUpdate();
	}
}

void UParkourMovementComponent::OpenWallRunGate()
{
	IsWallrunGateOpen = true;
}

void UParkourMovementComponent::CloseWallRunGate()
{
	IsWallrunGateOpen = false;
}

void UParkourMovementComponent::VerticalWallRunGate()
{
	if (IsVerticalWallrunGateOpen) {
		VerticalWallRunUpdate();
	}
}

void UParkourMovementComponent::OpenVerticalWallRunGate()
{
	// VerticalWallRunTime needs to be set in order to use this function.
	// If this value is 0, VerticalWallRun is Unlimited and will not fall.
	// If this does have a value, the character's vertical wall run will end after a set amount of time.

	if (VerticalWallRunTime > 0) {
		GetWorld()->GetTimerManager().SetTimer(VerticalWallRunEndEventHandle, this, &UParkourMovementComponent::VerticalWallRunEndEvent, VerticalWallRunTime, false);
	}

	IsVerticalWallrunGateOpen = true;
}

void UParkourMovementComponent::CloseVerticalWallRunGate()
{
	IsVerticalWallrunGateOpen = false;
	CloseMantleGate();
}

void UParkourMovementComponent::SlideGate()
{
	if (IsSlideGateOpen) {
		SlideUpdate();
	}
}

void UParkourMovementComponent::OpenSlideGate()
{
	IsSlideGateOpen = true;
}

void UParkourMovementComponent::CloseSlideGate()
{
	IsSlideGateOpen = false;
}

void UParkourMovementComponent::SprintGate()
{
	if (IsSprintGateOpen) {
		SprintUpdate();
	}
}

void UParkourMovementComponent::OpenSprintGate()
{
	IsSprintGateOpen = true;
}

void UParkourMovementComponent::CloseSprintGate()
{
	IsSprintGateOpen = false;
}

void UParkourMovementComponent::MantleCheck()
{
	if (MacroCanMantle()) {
		MantleStart();
	}
}

void UParkourMovementComponent::MantleCheckGate()
{
	if (IsMantleCheckGateOpen) {
		MantleCheck();
	}
}

void UParkourMovementComponent::OpenMantleCheckGate()
{
	IsMantleCheckGateOpen = true;
}

void UParkourMovementComponent::CloseMantleCheckGate()
{
	IsMantleCheckGateOpen = false;
}

void UParkourMovementComponent::MantleStart()
{
	if (SetParkourMovementMode(EParkourMovement::Mantle)) {

		if (MacroQuickMantle()) {
			// Broadcast QuickMantle Camera Shake
			//OnCameraShakeEvent.Broadcast();
		}
		else {
			// Broadcast Mantle Camera Shake
			//OnCameraShakeEvent.Broadcast();
		}
		CloseMantleCheckGate();
		OpenMantleGate();
	}
}

void UParkourMovementComponent::MantleGate()
{
	if (IsMantleGateOpen) {
		MantleMovement();
	}
}

void UParkourMovementComponent::OpenMantleGate()
{
	IsMantleGateOpen = true;
}

void UParkourMovementComponent::CloseMantleGate()
{
	IsMantleGateOpen = false;
}

/************************************************************/
/*------------------- Update Sequence ----------------------*/
/************************************************************/
void UParkourMovementComponent::UpdateSequence()
{
	WallRunGate();
	VerticalWallRunGate();
	MantleCheckGate();
	MantleGate();
	SlideGate();
	SprintGate();
	//CameraTick();
}

/************************************************************/
/*----------------------- Macros ---------------------------*/
/************************************************************/

/* Macro - Wall Running */
bool UParkourMovementComponent::MacroCanWallRun()
{
	bool results = false;
	float MyFloat = MacroForwardInput();
	bool MyBool = MacroWallRunning();
	bool MacroResults = CanWallRun(MyFloat, CurrentParkourMode, MyBool);

	if (MacroResults) {
		results = true;
	}

	return results;
}

bool UParkourMovementComponent::MacroWallRunning()
{
	bool results = false;

	if (WallRunning(CurrentParkourMode)) {
		results = true;
	}

	return results;
}

FVector UParkourMovementComponent::MacroWallRunEndVectorsLeft()
{
	return WallRunEndLeft(Character->GetActorLocation(), Character->GetActorRightVector(), Character->GetActorForwardVector());
}

FVector UParkourMovementComponent::MacroWallRunEndVectorsRight()
{
	return WallRunEndRight(Character->GetActorLocation(), Character->GetActorRightVector(), Character->GetActorForwardVector());
}

bool UParkourMovementComponent::MacroValidWallRunVector(FVector InVector)
{
	MacroOutValidWallRunVector = InVector;
	return ValidWallRunVector(InVector);
}

/* Macro - Input */
float UParkourMovementComponent::MacroForwardInput()
{
	return ForwardInput(Character->GetActorForwardVector(), Character->GetCharacterMovement()->GetLastInputVector());
}

/* Macro - Vertical Wall Running */
bool UParkourMovementComponent::MacroCanVerticalWallRun()
{
	bool results = false;
	float MyFloat = MacroForwardInput();
	bool MyBool = CharacterMovementComponent->IsFalling();
	bool MySecondBool = MacroWallRunning();

	results = CanVerticalWallRun(MyFloat, CurrentParkourMode, MyBool, MySecondBool);

	return results;
}

/* Macro - Mantling */
FVector UParkourMovementComponent::MacroMantleVectorsEyes()
{
	FVector EyesVector;
	FRotator EyesRotator;
	Character->GetController()->GetActorEyesViewPoint(EyesVector, EyesRotator);

	FVector ForwardVector = Character->GetActorForwardVector();

	FVector Results = ((EyesVector + FVector(0, 0, 50)) + (ForwardVector * 50.0f));

	//return Results;

	return MantleVectorEyes(EyesVector, ForwardVector);
}

FVector UParkourMovementComponent::MacroMantleVectorsFeet()
{
	FVector LocationVector = Character->GetActorLocation();
	float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float mHeight = MantleHeight;

	FVector ForwardVector = Character->GetActorForwardVector();

	FVector Results = ((LocationVector - FVector(0, 0, (CapsuleHalfHeight - mHeight))) + (ForwardVector * 50.0f));
	//return Results;

	return MantleVectorFeet(LocationVector, CapsuleHalfHeight, mHeight, ForwardVector);
}

bool UParkourMovementComponent::MacroCanMantle()
{
	return CanMantle(MacroForwardInput(), CurrentParkourMode, MacroQuickMantle());
}

bool UParkourMovementComponent::MacroQuickMantle()
{
	return QuickMantle(MantleTraceDistance, MantleHeight, LedgeCloseToGround);
}

/* Macro - Sliding */
bool UParkourMovementComponent::MacroCanSlide()
{
	return CanSlide(MacroForwardInput(), CurrentParkourMode, SprintQueued);
}

/* Macro - Sprinting */
bool UParkourMovementComponent::MacroCanSprint()
{
	return CanSprint(CharacterMovementComponent->IsWalking(), CurrentParkourMode);
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Math/Color.h"
#include "Parkour_C_Remake/Parkour_C_RemakeCharacter.h"

/************************************************************/
/*------------------ Initial Set Up ------------------------*/
/************************************************************/

// Sets default values for this component's properties
UParkourMovementComponent::UParkourMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	// Initialize Character, CharacterMovement Component, and the Previous and Current Parkour modes

	PreviousParkourMode = EParkourMovement::None;
	CurrentParkourMode = EParkourMovement::None;

	// Main Event Delegates
	this->OnMovementChanged.AddDynamic(this, &UParkourMovementComponent::MovementChanged);
	this->OnParkourChanged.AddDynamic(this, &UParkourMovementComponent::ParkourMovementChanged);	
	this->OnJumpEvent.AddDynamic(this, &UParkourMovementComponent::Jump);
	this->OnCameraShakeEvent.AddDynamic(this, &UParkourMovementComponent::PlayCameraShake);
	this->OnLandEvent.AddDynamic(this, &UParkourMovementComponent::Land);
	this->OnSprintEvent.AddDynamic(this, &UParkourMovementComponent::Sprint);
	this->OnQueuesCheckEvent.AddDynamic(this, &UParkourMovementComponent::CheckQueues);
	this->OnCrouchSlideEvent.AddDynamic(this, &UParkourMovementComponent::CrouchSlide);


	// In Program Events
	this->OnWallRunEnd.AddDynamic(this, &UParkourMovementComponent::WallRunEnd);
	this->OnUpdateEvent.AddDynamic(this, &UParkourMovementComponent::UpdateSequence);
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

	// ...
}

/************************************************************/
/*------------------------ Main ----------------------------*/
/************************************************************/

// Initialize Event called using Initialize Broadcast
void UParkourMovementComponent::Initialize(ACharacter* PlayerCharacter)
{
	UE_LOG(LogTemp, Warning, TEXT("Initialize has been called."));
	Character = PlayerCharacter;
	CharacterMovementComponent = PlayerCharacter->GetCharacterMovement();

	// Update Camera Properties
	DefaultUseControllerRotationYaw = PlayerCharacter->bUseControllerRotationYaw;

	DefaultGravity = CharacterMovementComponent->GravityScale;
	DefaultGroundFriction = CharacterMovementComponent->GroundFriction;
	DefaultMaxWalkSpeed = CharacterMovementComponent->MaxWalkSpeed;
	DefaultMaxCrouchSpeed = CharacterMovementComponent->MaxWalkSpeedCrouched;
	DefaultBrakingDeceleration = CharacterMovementComponent->BrakingDecelerationWalking;

	K2_SetTimerDelegate(UpdateEvent(), InitializeTime, true, 0, 0);
}

void UParkourMovementComponent::Land()
{
	EndEvents();
	CloseGates();
	// Play Camera Shake
}

void UParkourMovementComponent::Jump()
{
	// If No Current Parkour Mode is used, check if the character is falling.
	if (CurrentParkourMode == EParkourMovement::None) {
		// If Not falling, Call OpenGates and then the PlayCameraShake Event
		if (!CharacterMovementComponent->IsFalling()) {
			OpenGates();
			//OnCameraShakeEvent.Broadcast();
		}
	}
	else {
		// If in a Current Parkour Mode, go through the sequence of Jump Events.
		JumpEvent();
	}
}

void UParkourMovementComponent::PlayCameraShake()
{
	if (CameraShake) {
		// Play World Camera Shake with the Shake Camera Selected, Epicenter of Character Location
		// Inner Radius of 0, Outer Radius of 100, Falloff at 1, and No Orient Shake Towards Epicenter
		UE_LOG(LogTemp, Warning, TEXT("Played Camera Shake"));
	}
}

void UParkourMovementComponent::CrouchSlide()
{
	CancelMovement();

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

void UParkourMovementComponent::CheckQueues()
{
	if (SlideQueued) {
		SlideStart();
	}
	else if (SprintQueued) {
		SprintStart();
	}
}

FTimerHandle UParkourMovementComponent::K2_SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay, float InitialStartDelayVariance)
{
	return FTimerHandle();
}

void UParkourMovementComponent::MovementChanged(EMovementMode PrevMovementMode, EMovementMode NewMovementMode)
{
	if (IsValid(CharacterMovementComponent)) {
		PreviousMovementMode = PrevMovementMode;
		CurrentMovementMode = NewMovementMode;

		OpenGates();
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
		results = false;
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
		case EParkourMovement::None: EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::LeftWallRun: EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::RightWallRun: EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::VerticalWallRun: EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::LedgeGrab: EMovementMode::MOVE_Falling;
			break;
		case EParkourMovement::Mantle: EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::Slide: EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::Crouch: EMovementMode::MOVE_Walking;
			break;
		case EParkourMovement::Sprint: EMovementMode::MOVE_Walking;
			break;
		}
		CharacterMovementComponent->SetMovementMode(NewMode, 0);
	}
	else {
		CharacterMovementComponent->bOrientRotationToMovement = (CurrentParkourMode == EParkourMovement::Sprint);

		Character->bUseControllerRotationYaw = ((CurrentParkourMode == EParkourMovement::Sprint) && DefaultUseControllerRotationYaw);
	}
}

void UParkourMovementComponent::CancelMovement()
{
	if ((CurrentParkourMode == EParkourMovement::LedgeGrab) || (CurrentParkourMode == EParkourMovement::VerticalWallRun) || (CurrentParkourMode == EParkourMovement::Mantle)) {
		VerticalWallRunEnd(0.5);
	}
	else if (MacroWallRunning()) {
		WallRunEnd(0.5);
	}
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

FTimerDynamicDelegate UParkourMovementComponent::UpdateEvent()
{
	OnUpdateEvent.Broadcast();
	return FTimerDynamicDelegate();
}

/************************************************************/
/*---------------------- Wall Run --------------------------*/
/************************************************************/

void UParkourMovementComponent::WallRunUpdate()
{
	if (MacroCanWallRun()) {
		// Call Function WallRunMovement With Character's Location Vector, the Wall Run End Right Vector, and Run Direction of -1.0. Returns a Boolean.

		if (WallRunMovement(Character->GetActorLocation(), MacroWallRunEndVectorsRight(), -1.0)) {
			if (SetParkourMovementMode(EParkourMovement::RightWallRun)) {
				FString WallRunEnableGravity = "WallRunEnableGravity";
				// Call Function Set Timer By Function Name, with function name WallRunEnableGravity, and a float time set as 1
				UKismetSystemLibrary::K2_SetTimer(this, WallRunEnableGravity, 1.0, false, 0.0, 0.0);

				// Call Delegate Correct Wall Run Location
				CorrectWallRunLocation();
				// Call Delegate Wall Run Gravity
				// WallRunEnableGravity();
			}
			else {
				// Call Delegate Wall Run Gravity
				// WallRunEnableGravity();
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
						FString WallRunEnableGravity = "WallRunEnableGravity";
						// Call Function Set Timer By Function Name, with function name WallRunEnableGravity, and a float time set as 1
						UKismetSystemLibrary::K2_SetTimer(this, WallRunEnableGravity, 1.0, false, 0.0, 0.0);

						// Call Delegate Correct Wall Run Location
						CorrectWallRunLocation();
						// Call Delegate Wall Run Gravity
						// WallRunEnableGravity();
					}
					else {
						// Call Delegate Wall Run Gravity
						// WallRunEnableGravity();
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

void UParkourMovementComponent::WallRunEnd(float ResetTime)
{
	if (MacroWallRunning()) {
		if (SetParkourMovementMode(EParkourMovement::None)) {
			FString OpenWallRunGate = "OpenWallRunGate";
			FString WallRunEnableGravity = "WallRunEnableGravity";
			// Call Close Wall Run Gate Event Dispatcher
			CloseWallRunGate();
			// Call Function Set Timer By Function Name, with function name OpenWallRunGate, and the time set as Reset Time)
			// UKismetSystemLibrary::K2_SetTimer(UObject* Object, FString FunctionName, float Time, bool bLooping, float InitialStartDelay, float InitialStartDelayVariance)
			UKismetSystemLibrary::K2_SetTimer(this, OpenWallRunGate, ResetTime, false, 0.0, 0.0);

			// Call Function Clear Timer by Function Name, with function name as WallRunEnableGravity
			// UKismetSystemLibrary::K2_ClearTimer(UObject* Object, FString FunctionName)
			UKismetSystemLibrary::K2_ClearTimer(this, WallRunEnableGravity);

			// Set Wall Run Gravity to false;
			bIsWallRunGravity = false;
		}
		else {
			return;
		}
	}
	else {
		return;
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

bool UParkourMovementComponent::WallRunEnableGravity()
{
	if (MacroWallRunning()) {
		return true;
	}
	else {
		return false;
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

/************************************************************/
/*----------------- Vertical Wall Run ----------------------*/
/************************************************************/

void UParkourMovementComponent::VerticalWallRunUpdate()
{
	if (MacroCanVerticalWallRun()) {
		FHitResult HitResults;
		
		FLinearColor ColorOne = FLinearColor(0, 1, 0, 1);
		FLinearColor ColorTwo = FLinearColor(1, 0, 0, 0);

		if (UKismetSystemLibrary::CapsuleTraceSingle(Character->GetCapsuleComponent(), MacroMantleVectorsEyes(), MacroMantleVectorsFeet(), 20, 10, TraceTypeQuery4, false, ActorsToIgnore, EDrawDebugTrace::Type::None, HitResults, true, ColorTwo, ColorOne, 5)) {

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

				if (UKismetSystemLibrary::LineTraceSingle(this, Start, End, TraceTypeQuery4, false, ActorsToIgnore, EDrawDebugTrace::Type::None, LineHitResult, true, FLinearColor(1, 0, 0, 1), FLinearColor(0, 1, 0, 1), 5)) {
					LedgeCloseToGround = true;

					if (MacroQuickMantle()) {
						OpenMantleCheckGate();
					}
					else {
						CorrectLedgeLocation();

						FString OpenMantleCheckGate = "OpenMantleCheckGate";
						UKismetSystemLibrary::K2_SetTimer(this, OpenMantleCheckGate, 0.25, false);
					}
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
			// Close Mantle Check Gate
			LedgeCloseToGround = false;

			FString OpenVerticalWallRunGate = "OpenVerticalWallRunGate";
			FString CheckQueues = "CheckQueues";

			UKismetSystemLibrary::K2_SetTimer(this, OpenVerticalWallRunGate, ResetTime, false, 0.0, 0.0);
			UKismetSystemLibrary::K2_SetTimer(this, CheckQueues, 0.02, false, 0.0, 0.0);

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

			FVector LaunchResults = FVector(VerticalWallRunNormal.X * -600, VerticalWallRunNormal.Y * -600, VerticalWallRunSpeed);
			Character->LaunchCharacter(LaunchResults, true, true);
		}
		else {
			FVector LaunchResults = FVector(VerticalWallRunNormal.X * -600, VerticalWallRunNormal.Y * -600, VerticalWallRunSpeed);
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
	FRotator results = UKismetMathLibrary::MakeRotator(Character->GetCapsuleComponent()->GetRelativeRotation().Roll, Character->GetCapsuleComponent()->GetRelativeRotation().Pitch, Yaw);
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
	FRotator results = UKismetMathLibrary::MakeRotator(Character->GetCapsuleComponent()->GetRelativeRotation().Roll, Character->GetCapsuleComponent()->GetRelativeRotation().Pitch, (YawResults.Yaw - 180));
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

FTimerDynamicDelegate UParkourMovementComponent::VerticalWallRunEndEvent()
{
	FString VerticalWallRunEndEvent = "VerticalWallRunEndEvent";
	UKismetSystemLibrary::K2_ClearTimer(this, VerticalWallRunEndEvent);
	if (CurrentParkourMode == EParkourMovement::VerticalWallRun) {
		VerticalWallRunEnd(2);
	}
	return FTimerDynamicDelegate();
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
		// PlayCameraShake(); // CameraShake Ledge Grab
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
			//SetTimerByFunctionName - OpenSprintGate
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
	FRotator InterpR = UKismetMathLibrary::RInterpTo(CurrentRotator, TargetRotator, GetWorld()->DeltaTimeSeconds, 7);
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

}

void UParkourMovementComponent::CameraTilt(float TargetXRoll)
{

}

void UParkourMovementComponent::CameraTick()
{

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
	IsVerticalWallrunGateOpen = true;

	if (VerticalWallRunTime > 0) {
		UKismetSystemLibrary::K2_SetTimerDelegate(VerticalWallRunEndEvent(), VerticalWallRunTime, false, 0.0, 0.0);
	}
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
		//PlayCameraShake();
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
void UParkourMovementComponent::CameraTickSequence()
{

}

void UParkourMovementComponent::UpdateSequence()
{
	WallRunGate();
	VerticalWallRunGate();
	MantleCheckGate();
	MantleGate();
	SlideGate();
	SprintGate();
	//CameraTickSequence();
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
	Character->GetController()->GetActorEyesViewPoint(EyesVector,EyesRotator);

	FVector ForwardVector = Character->GetActorForwardVector();

	
	return MantleVectorEyes(EyesVector, ForwardVector);
}

FVector UParkourMovementComponent::MacroMantleVectorsFeet()
{
	FVector LocationVector = Character->GetActorLocation();
	float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float mHeight = MantleHeight;
	FVector ForwardVector = Character->GetActorForwardVector();

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
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/Vector.h"
#include "Camera/CameraShakeBase.h"

#include "ParkourMovementComponent.generated.h"

//class UMatineeCameraShake;

// Macro Definitions
#define ForwardInput(MyVector1, MyVector2) (FVector::DotProduct(MyVector1, MyVector2));
#define WallRunning(CParkourMode) ((CParkourMode == EParkourMovement(1)) || (CParkourMode == EParkourMovement(2)))
#define CanWallRun(MyFloat, CParkourMode, MyBool) ((MyFloat > 0) && ((CParkourMode == EParkourMovement(0)) || MyBool == true));
#define WallRunEndRight(ActorLocation, ActorRightVector, ActorForwardVector) (ActorLocation + (ActorRightVector * 75.f) + (ActorForwardVector * -35.f));
#define WallRunEndLeft(ActorLocation, ActorRightVector, ActorForwardVector) (ActorLocation + (ActorRightVector * -75.f) + (ActorForwardVector * -35.f));
#define ValidWallRunVector(MyVector) (MyVector.Z < 0.52 && MyVector.Z > -0.52)
#define CanVerticalWallRun(MyFloat, CParkourMode, MyBool, MySecondBool) ((MyFloat > 0) && (MyBool) && ((CParkourMode == EParkourMovement::None) || (CParkourMode == EParkourMovement::VerticalWallRun) || (MySecondBool)))
#define MantleVectorEyes(MyEyesVector, MyForwardVector) ((MyEyesVector + (0, 0, 50)) + (MyForwardVector * 50))
#define MantleVectorFeet(MyLocationVector, MyFloat, MySecondFloat, MyForwardVector) ((MyLocationVector + (0, 0, (MyFloat - MySecondFloat))) + (MyForwardVector * 50))
#define CanMantle(MyFloat, CParkourMode, MyBool) ((MyFloat > 0) && ((CParkourMode == EParkourMovement::LedgeGrab) || MyBool))
#define QuickMantle(MyFloat, MySecondFloat, MyBool) ((MyFloat > MySecondFloat) || MyBool)
#define CanSlide(MyFloat, CParkourMode, MyBool) ((MyFloat > 0) && ((CParkourMode == EParkourMovement::Sprint) || MyBool))
#define CanSprint(MyBool, CParkourMode) (MyBool && (CParkourMode == EParkourMovement::None))

UENUM(BlueprintType)
enum class EParkourMovement : uint8 {
	None = 0 UMETA(DisplayName = "None"),
	LeftWallRun = 1 UMETA(DisplayName = "LeftWallRun"),
	RightWallRun = 2 UMETA(DisplayName = "RightWallRun"),
	VerticalWallRun = 3 UMETA(DisplayName = "VerticalWallRun"),
	LedgeGrab = 4 UMETA(DisplayName = "LedgeGrab"),
	Mantle = 5 UMETA(DisplayName = "Mantle"),
	Slide = 6 UMETA(DisplayName = "Slide"),
	Crouch = 7 UMETA(DisplayName = "Crouch"),
	Sprint = 8 UMETA(DisplayName = "Sprint")
};

// Event Dispatchers
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMovementChangedDelegate, EMovementMode, PrevMovementMode, EMovementMode, NewMovementMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FParkourMovementChangedDelegate, EParkourMovement, PrevParkourMode, EParkourMovement, NewParkourMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWallRunEndDelegate, float, ResetTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUpdateEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJumpEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSprintEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCrouchSlideEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQueuesEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLandEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCameraShakeDelegate); // UMatineeCameraShake, CameraShake

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARKOUR_C_REMAKE_API UParkourMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UParkourMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/* Update Event */
	static FTimerHandle K2_SetTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping, float InitialStartDelay, float InitialStartDelayVariance);
	float InitializeTime = 0.0167;

	UFUNCTION(BlueprintCallable)
	FTimerDynamicDelegate UpdateEvent();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	ACharacter* Character;
	UCharacterMovementComponent* CharacterMovementComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Parkour")
	EParkourMovement PreviousParkourMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Parkour")
	EParkourMovement CurrentParkourMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Parkour")
	TEnumAsByte<EMovementMode> PreviousMovementMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Parkour")
	TEnumAsByte<EMovementMode> CurrentMovementMode;

	/* Main Events */
	/* Character Defaults */
	UFUNCTION(BlueprintCallable)
	void Initialize(ACharacter* PlayerCharacter);

	UFUNCTION(BlueprintCallable)
	void Land();

	UFUNCTION(BlueprintCallable)
	void Jump();

	UFUNCTION(BlueprintCallable)
	void PlayCameraShake();

	UFUNCTION(BlueprintCallable)
	void CrouchSlide();

	UFUNCTION(BlueprintCallable)
	void CheckQueues();

	UFUNCTION(BlueprintCallable)
	void MovementChanged(EMovementMode PrevMovementMode, EMovementMode NewMovementMode);

	UFUNCTION(BlueprintCallable)
	void ParkourMovementChanged(EParkourMovement PrevParkourMode, EParkourMovement NewParkourMode);
	
	UFUNCTION(BlueprintCallable)
	void Sprint();

	UFUNCTION(BlueprintCallable)
	bool SetParkourMovementMode(EParkourMovement NewMode);

	UFUNCTION(BlueprintCallable)
	void ResetMovement();

	UFUNCTION(BlueprintCallable)
	void CancelMovement();	

	UFUNCTION(BlueprintCallable)
	bool ForwardTracer(FHitResult& OutResult);	


	/* Delegates */
	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FMovementChangedDelegate OnMovementChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EventDispatcher")
	FParkourMovementChangedDelegate OnParkourChanged;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FWallRunEndDelegate OnWallRunEnd;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FUpdateEventDelegate OnUpdateEvent;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FJumpEventDelegate OnJumpEvent;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FCameraShakeDelegate OnCameraShakeEvent;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FLandEventDelegate OnLandEvent;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FSprintEventDelegate OnSprintEvent;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FCrouchSlideEventDelegate OnCrouchSlideEvent;

	UPROPERTY(BlueprintAssignable, Category = "EventDispatcher")
	FQueuesEventDelegate OnQueuesCheckEvent;

protected:
	/* C++ Specific Variables */
	const TArray<AActor*> ActorsToIgnore;

	/* Default Variables */
	//Defaults
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Default Variables")
	float DefaultGroundFriction = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Default Variables")
	float DefaultGravity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Default Variables")
	float DefaultMaxWalkSpeed = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Default Variables")
	float DefaultMaxCrouchSpeed = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Default Variables")
	float DefaultBrakingDeceleration = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Default Variables")
	bool DefaultUseControllerRotationYaw = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Default Variables")
	bool CameraShake = false;

	/* Gates */ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Gates")
	bool IsWallrunGateOpen = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Gates")
	bool IsSprintGateOpen = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Gates")
	bool IsSlideGateOpen = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Gates")
	bool IsVerticalWallrunGateOpen = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Gates")
	bool IsMantleCheckGateOpen = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Gates")
	bool IsMantleGateOpen = false;

	/* Variables */
	//Wall Run Variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	FVector WallRunNormal = FVector(0,0,0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float WallRunTargetGravity = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float WallRunStartSpeed = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float WallRunJumpHeight = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float WallRunJumpOffForce = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float WallRunSpeed = 850.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float WallRunSprintSpeed = 1100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	bool bIsWallRunGravity = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float VerticalWallRunSpeed = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	FVector VerticalWallRunNormal = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	FVector VerticalWallRunLocation = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	FVector WallRunLocation = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | WallRun")
	float VerticalWallRunTime = 0.0f;

	//LedgeGrab Variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	float LedgeGrabJumpOffForce = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	float LedgeGrabJumpHeight = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	FVector LedgeFloorPosition = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	bool LedgeCloseToGround = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	FVector LedgeClimbWallPosition = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	FVector LedgeClimbWallNormal = FVector(0, 0, 0);

	//Mantle Variables
	FVector MantlePosition = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	float MantleSpeed = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	float MantleTraceDistance = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	float QuickMantleSpeed = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | LedgeGrab")
	float MantleHeight = 44.0f;


	//Slide Variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Slide")
	FVector SlideVector = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Slide")
	float SlideImpulseAmount = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Slide")
	bool SlideQueued = false;

	//Sprint Variables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Sprint")
	float SprintSpeed = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ParkourMovement | Sprint")
	bool SprintQueued = false;

	//Matinee Camera Variables
	/*UPROPERTY(EditAnywhere, Category = "Effects")
	TSubclassOf<UMatineeCameraShake> JumpCamera;*/

private:
	/* Wall Run */
	void WallRunUpdate();
	
	// Delegate
	UFUNCTION()
	void WallRunEnd(float ResetTime);

	bool WallRunMovement(FVector Start, FVector End, float WallRunDirection);
	void WallRunGravity();	
	bool WallRunEnableGravity();
	void CorrectWallRunLocation();
	FVector WallRunTargetVector();
	FRotator WallRunTargetRotation();

	/* Vertical Wall Run */
	void VerticalWallRunUpdate();
	void VerticalWallRunEnd(float ResetTime);
	void VerticalWallRunMovement();

	void CorrectVerticalWallRunLocation();
	FVector VerticalWallRunTargetLocation();
	FRotator VerticalWallRunTargetRotation();

	void CorrectLedgeLocation();
	FVector LedgeTargetLocation();
	FRotator LedgeTargetRotation();

	float MantleZOffset();
	float CapsuleZOffset();

	UFUNCTION()
	FTimerDynamicDelegate VerticalWallRunEndEvent();

	/* Jump */
	void WallRunJump();
	void LedgeGrabJump();
	void SlideJump();	
	void CrouchJump();
	void SprintJump();

	/* Ledge Grab */
	void LedgeGrab();

	/* Sprint */
	void SprintUpdate();
	void SprintEnd();
	void SprintStart();

	/* Crouch */
	void CrouchStart();
	void CrouchEnd();
	void CrouchToggle();

	/* Slide */
	void SlideUpdate();
	void SlideStart();
	void SlideEnd(bool IsCrouched);

	FVector VelocityNormal();
	FVector GetSlideVector();

	/* Mantle */
	void MantleMovement();

	/* Camera */
	void UpdateCameraProperties();
	void CameraTilt(float TargetXRoll);
	void CameraTick();

	/* Events */
	void EndEvents(); //End WallRun -> Verti WallRun -> Slide -> Sprint
	void JumpEvent(); ////Jump WallRun -> LedgeGrab -> Slide -> Crouch -> Sprint (Needs to be changed to JumpSequence)

	/* Gates */
	void OpenGates(); //Open WallRun -> Verti WallRun -> Slide -> Sprint
	void CloseGates(); //Close WallRun -> Verti WallRun -> Slide -> Sprint
	void OpenMovementGates();

	// Wallrun
	void WallRunGate();
	void OpenWallRunGate();
	void CloseWallRunGate();

	// Vertical Wall Run
	void VerticalWallRunGate();
	void OpenVerticalWallRunGate();
	void CloseVerticalWallRunGate();

	// Slide
	void SlideGate();
	void OpenSlideGate();
	void CloseSlideGate();

	// Sprint
	void SprintGate();
	void OpenSprintGate();
	void CloseSprintGate();

	//Mantle
	void MantleCheck();
	void MantleCheckGate();
	void OpenMantleCheckGate();
	void CloseMantleCheckGate();
	void MantleStart();
	void MantleGate();
	void OpenMantleGate();
	void CloseMantleGate();

	/* Gate Sequences */

	//Delegates
	UFUNCTION()
	void UpdateSequence();

	void CameraTickSequence();

	/* Macros */
	bool MacroCanWallRun();
	bool MacroWallRunning();
	bool MacroCanVerticalWallRun();
	bool MacroValidWallRunVector(FVector);
	FVector MacroOutValidWallRunVector = {0, 0, 0};
	FVector MacroWallRunEndVectorsLeft();
	FVector MacroWallRunEndVectorsRight();

	float MacroForwardInput();	

	FVector MacroMantleVectorsEyes();
	FVector MacroMantleVectorsFeet();
	bool MacroCanMantle();
	bool MacroQuickMantle();

	bool MacroCanSlide();

	bool MacroCanSprint();
};

/*
* To Do:
* Mantle Event
* ParkourCancel - Check this
* Find out what to do about FTimerDynamicDelegate
* 
* Events To Potentially Be Broadcast/Called:
* WallRunEnableGravity
* CorrectWallRunLocation
* CorrectVerticalWallRunLocation
* CorrectLedgeLocation
* VerticalWallRunEndEvent
* SprintEvent, SprintOpen, SprintClose
* CrouchToggle
* SlideEvent, SlideOpen, SlideClose
* JumpEvents, EndEvents
* OpenGates, CloseGates
* OpenMovementGates
* 
*/
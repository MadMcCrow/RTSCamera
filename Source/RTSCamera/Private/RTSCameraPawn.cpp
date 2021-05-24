/* Copyright © Noé Perard-Gayot 2021. Licensed under the MIT License. You may obtain a copy of the License at https://opensource.org/licenses/mit-license.php */

#include "RTSCameraPawn.h"
#include "Components/RTSCameraComponent.h"
#include "Components/RTSCameraMovementComponent.h"
#include "Components/RTSCameraMouseComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Components static Names
FName ARTSCameraPawn::CameraComponentName	 = FName("CameraComponent");
FName ARTSCameraPawn::MouseComponentName		 = FName("MouseComponent");
FName ARTSCameraPawn::CameraBoomComponentName = FName("CameraBoomComponent");

// Sets default values, avoid the spectator construct
ARTSCameraPawn::ARTSCameraPawn(const FObjectInitializer& ObjectInitializer) : Super( ObjectInitializer.SetDefaultSubobjectClass<URTSCameraMovementComponent>(Super::MovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// Setting parameters default values
	ZoomSpeed = 5.f;
	BaseMovementSpeed = 10.f;
	MaxZoom = 7000.f;
	MinZoom = 300.f;
	
	// Create Components
	CameraComponent		= ObjectInitializer.CreateDefaultSubobject<URTSCameraComponent>(this, CameraComponentName);
	CameraBoomComponent = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, CameraBoomComponentName);
	MouseComponent		= ObjectInitializer.CreateDefaultSubobject<URTSCameraMouseComponent>(this, MouseComponentName);

	// Setup attachement
	CameraBoomComponent->SetupAttachment(RootComponent);
	CameraComponent->SetupAttachment(CameraBoomComponent);

	CameraBoomComponent->bDoCollisionTest = false;
	CameraBoomComponent->TargetArmLength = 700;

}

// Called when the game starts or when spawned
void ARTSCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	if (MouseComponent)
	{
		MouseComponent->OnEnteredBorder.AddUniqueDynamic(this, &ARTSCameraPawn::StartMouseMovement);
		MouseComponent->OnLeftBorder.AddUniqueDynamic(this, &ARTSCameraPawn::EndMouseMovement);
	}
	if (CameraBoomComponent)
	{
		ZoomDistance = CameraBoomComponent->TargetArmLength;
	}
}

// Called every frame
void ARTSCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMoveBorder && MouseComponent)
	{
		float scale = 0;
		FVector2D border = MouseComponent->GetBorderDirection();
		border.ToDirectionAndLength(border, scale);
		AddMovementInput(FVector(border, 0.f), scale * scale * GetMovementSpeed() * DeltaTime );
	}

	CameraTracking();

}

// Called to bind functionality to input
void ARTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	PlayerInputComponent->BindAxis(InputAxisForwardName, this, &ARTSCameraPawn::MoveAxisForward).bConsumeInput = false;
	PlayerInputComponent->BindAxis(InputAxisRightName,   this, &ARTSCameraPawn::MoveAxisRight).bConsumeInput = false;
	PlayerInputComponent->BindAxis(InputAxisZoomName,	 this, &ARTSCameraPawn::Zoom).bConsumeInput = false;
}

void ARTSCameraPawn::SetFreeCamera(bool IsFree)
{
	bIsFreeCamera = IsFree;
}

void ARTSCameraPawn::FollowActor(AActor* Target)
{
	if (Target)
	{
		SetFreeCamera(false);
	}
	else
	{
		SetFreeCamera(true);
	}
}

void ARTSCameraPawn::MoveAxisForward(float AxisValue)
{
	AddMovementInput(GetActorForwardVector(), AxisValue * GetMovementSpeed());
	SetFreeCamera(true);
}

void ARTSCameraPawn::MoveAxisRight(float AxisValue)
{
	AddMovementInput(GetActorRightVector(), AxisValue );
	SetFreeCamera(true);
}

void ARTSCameraPawn::Zoom(float AxisValue)
{
	if (CameraBoomComponent)
	{
		ZoomDistance = FMath::Clamp(ZoomDistance + AxisValue * -1 * (ZoomSpeed * FMath::Sqrt(ZoomDistance)), MinZoom, MaxZoom);
		CameraBoomComponent->TargetArmLength = FMath::FInterpTo(CameraBoomComponent->TargetArmLength, ZoomDistance, GetWorld()->GetDeltaSeconds(), 5.0);
	}
	SetFreeCamera(true);
}

void ARTSCameraPawn::CameraTracking()
{
	if (!bIsFreeCamera && TrackedActor)
	{
		FVector direction;
		float length;
		(TrackedActor->GetActorLocation() - GetActorLocation()).ToDirectionAndLength(direction, length);
		AddMovementInput(direction, FMath::Min<float>(length, GetMovementSpeed()));
	}
}

float ARTSCameraPawn::GetMovementSpeed_Implementation() const
{
	return FMath::Sqrt(ZoomDistance) * BaseMovementSpeed;
}

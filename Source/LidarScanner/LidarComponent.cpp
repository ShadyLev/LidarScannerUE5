// Fill out your copyright notice in the Description page of Project Settings.


#include "LidarComponent.h"
#include "LidarScannerCharacter.h"
#include "LidarScannerProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "LidarDataInterface.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"


// Sets default values for this component's properties
ULidarComponent::ULidarComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
}


// Called when the game starts
void ULidarComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULidarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ensure we have a character owner
	if (Character != nullptr)
	{
		// remove the input mapping context from the Player Controller
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(FireMappingContext);
			}
		}
	}

	// maintain the EndPlay call chain
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ULidarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFullScan(DeltaTime);
}

#pragma region NormalScan
void ULidarComponent::NormalScan()
{
	const UWorld* World = GetWorld();

	if(World == nullptr)
		return;
	
	FHitResult Hit;

	PositionArray.Empty();
	ColorArray.Empty();
	LifetimeArray.Empty();

	for (int i = 0; i < ScanRayAmount; ++i)
	{
		const FVector2D RandomOffset = GetRandomPointInsideCircle(ScanRadius);

		const APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		FVector Start = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);

		FVector ForwardVector = FRotationMatrix(SpawnRotation).GetUnitAxis(EAxis::X);
		FVector RightVector = FRotationMatrix(SpawnRotation).GetUnitAxis(EAxis::Y);
		FVector UpVector = FRotationMatrix(SpawnRotation).GetUnitAxis(EAxis::Z);
		
		FVector Direction = ForwardVector + (RightVector * RandomOffset.X) + (UpVector * RandomOffset.Y);
		Direction = Direction.GetSafeNormal();

		const bool bHit = LineCast(Start, Direction, Hit);
		if (bHit == false)
			continue;

		AddParticleData(Hit);
	}

	SetNiagaraParticleData();
}

FVector2D ULidarComponent::GetRandomPointInsideCircle(float Radius)
{
	const float a = FMath::RandRange(0.f, 2.f * PI);
	const float r = FMath::RandRange(0.f, Radius);

	const float x = FMath::Sqrt(r * Radius) * FMath::Cos(a);
	const float y = FMath::Sqrt(r * Radius) * FMath::Sin(a);

	return FVector2D(x, y);
}

void ULidarComponent::AdjustScanRadius(const FInputActionValue& Value)
{
	const float InputValue = Value.Get<float>();
	
	float Delta = InputValue > 0.f ? 1.f : -1.f;
	Delta *= ScanRadiusChangeMultiplier;

	ScanRadius += Delta;

	ScanRadius = FMath::Clamp(ScanRadius, ScanRadiusMin, ScanRadiusMax);
}

#pragma endregion

FLinearColor ULidarComponent::LerpColors(float Distance)
{
	// Clamp Alpha to ensure it is between 0.0 and 1.0
	const float Alpha = FMath::Clamp(Distance / ParticleColorMaxDistance, 0.0f, 1.0f);
	return FLinearColor::LerpUsingHSV(ParticleColorClose, ParticleColorFar, Alpha);
}

#pragma region FullScan

void ULidarComponent::StartFullScan()
{
	if (FullScanInProgress)
		return;

	FullScanCurrentAngle = -FullScanVerticalAngle;
	FullScanInProgress = true;

	if(EnableDebug)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("STARTED FULL SCAN"));
}

void ULidarComponent::UpdateFullScan(const float DeltaTime)
{
	if (FullScanInProgress == false)
		return;

	if(FullScanCurrentAngle + FullScanRate * DeltaTime >= FullScanVerticalAngle)
	{
		FullScanInProgress = false;
		return;
	}

	FullScanCurrentAngle += FullScanRate * DeltaTime;


	PerformFullScan();
}

void ULidarComponent::PerformFullScan()
{
	if (UWorld* const World = GetWorld(); World == nullptr)
		return;

	FHitResult Hit;

	PositionArray.Empty();
	ColorArray.Empty();
	LifetimeArray.Empty();
	
	const float HorizontalStep = FullScanHorizontalAngle / static_cast<float>(FullScanRayAmount);

	for (int i = 0; i < FullScanRayAmount; i++)
	{
		float BaseYaw = -FullScanHorizontalAngle / 2 + i * HorizontalStep;

		float RandomDeviation = FMath::RandRange(-HorizontalStep / 2, HorizontalStep / 2);
		float YawDeg = BaseYaw + RandomDeviation;

		const APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		const FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		const FVector StartLocation = GetOwner()->GetActorLocation() + CameraRotation.RotateVector(MuzzleOffset);
		FVector Direction = GetScanDirection(FullScanCurrentAngle, YawDeg, CameraRotation);
		if (LineCast(StartLocation, Direction, Hit))
		{
			AddParticleData(Hit);
		}
	}
	
	SetNiagaraParticleData();
}

FVector ULidarComponent::GetScanDirection(float VerticalAngle, float HorizontalDegrees, FRotator CameraRotation) const
{
	const FRotator NewRotation(VerticalAngle, HorizontalDegrees, 0);

	return NewRotation.RotateVector(FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::X));	
}


#pragma endregion

void ULidarComponent::AddParticleData(FHitResult& Hit)
{
	// Every particle needs a position
	PositionArray.Add(Hit.Location);

	if (FCustomParticleData Data; GetParticleDataFromTag(Hit.Component->ComponentTags, Data))
	{
		ColorArray.Add(Data.Color);
		LifetimeArray.Add(Data.Lifetime);
		return;
	}

	// Default behaviour
	FLinearColor ParticleColor = LerpColors(Hit.Distance);
	ColorArray.Add(ParticleColor);
	LifetimeArray.Add(DefaultParticleLifetime);
}

bool ULidarComponent::GetParticleDataFromTag(TArray<FName>& Tags, FCustomParticleData& Data)
{
	for (FName Tag : Tags)
	{
		if(CustomDataDictionary.Contains(Tag) == false)
		{
			continue;
		}

		Data = CustomDataDictionary[Tag];
		return true;
	}
	return false;
}


bool ULidarComponent::LineCast(const FVector& Start, const FVector& Direction, FHitResult& Hit) const
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(Character);

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Direction + Direction * RaycastLength, ECC_Camera, QueryParams);

	if (EnableDebug)
	{
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), Hit.Location, 2.f, 2, FColor::Green, false, LineTraceLinger);
			DrawDebugLine(GetWorld(), Start, Hit.Location, FColor::Green, false, LineTraceLinger);
		}
		else
		{
			DrawDebugLine(GetWorld(), Start, Start + Direction * RaycastLength, FColor::Red, false, LineTraceLinger);
		}
	}

	return bHit;
}



#pragma region Niagara
void ULidarComponent::InitializeNiagaraSystem()
{
	if (NiagaraSystemAsset)
	{
		NiagaraComponent->SetAsset(NiagaraSystemAsset);
		NiagaraComponent->Activate();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NiagaraSystemAsset is not set on %s"), *GetName());
	}
}

void ULidarComponent::SetNiagaraParticleData()
{
	if (NiagaraComponent)
	{
		// We should make these variables exposed to BP maybe, dont like it hardcoded like this
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComponent, "ParticlePositions", PositionArray);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(NiagaraComponent, "ParticleColors", ColorArray);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(NiagaraComponent, "ParticleLifetimes", LifetimeArray);
	}
}

#pragma endregion

bool ULidarComponent::AttachScanner(ALidarScannerCharacter* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no weapon component yet
	if (Character == nullptr || Character->GetInstanceComponents().FindItemByClass<ULidarComponent>())
	{
		return false;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Bind Inputs
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ULidarComponent::NormalScan);
			EnhancedInputComponent->BindAction(FullScanAction, ETriggerEvent::Triggered, this, &ULidarComponent::StartFullScan);
			EnhancedInputComponent->BindAction(AdjustScanRadiusAction, ETriggerEvent::Triggered, this, &ULidarComponent::AdjustScanRadius);
		}
	}

	return true;
}



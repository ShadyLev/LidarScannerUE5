// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LidarScannerCharacter.h"
#include "NiagaraComponent.h"
#include "ParticleStruct.h"
#include "Components/SceneComponent.h"
#include "Public/CustomParticleData.h"
#include "LidarComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LIDARSCANNER_API ULidarComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULidarComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Scanner")
	bool AttachScanner(ALidarScannerCharacter* TargetCharacter);

private:
	/** The Character holding this weapon*/
	ALidarScannerCharacter* Character;

public:
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FullScanAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* AdjustScanRadiusAction;
	
public:
	UPROPERTY(EditAnywhere ,Category="General Variables")
	float RaycastLength = 10000.f;
	UPROPERTY(EditAnywhere ,Category="General Variables")
	bool EnableDebug = true;
	UPROPERTY(EditAnywhere ,Category="General Variables")
	float LineTraceLinger = 1.f;
private:
	TArray<FParticleStruct> Particles;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly ,Category="VFX")
	UNiagaraComponent* NiagaraComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="VFX")
	UNiagaraSystem* NiagaraSystemAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
	FVector MuzzleOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
	float DefaultParticleLifetime = 99999.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
	float ParticleColorMaxDistance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
	FLinearColor ParticleColorClose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
	FLinearColor ParticleColorFar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TMap<FName, FCustomParticleData> CustomDataDictionary;

	UFUNCTION(BlueprintCallable)
	void SetNiagaraParticleData(TArray<FVector>& Positions, TArray<FLinearColor>& Colors, TArray<float>& Lifetimes);

	UFUNCTION(BlueprintCallable)
	void InitializeNiagaraSystem();
	
	UFUNCTION(BlueprintCallable)
	FLinearColor LerpColors(float Distance);

	UFUNCTION(BlueprintCallable)
	bool GetParticleDataFromTag(TArray<FName>& Tags, FCustomParticleData& Data);
private:

public:
	UPROPERTY(EditAnywhere ,Category="Normal Scan")
	int ScanRayAmount = 50.f;
	UPROPERTY(EditAnywhere ,Category="Normal Scan")
	float ScanRadiusMin = 0.1f;
	UPROPERTY(EditAnywhere ,Category="Normal Scan")
	float ScanRadiusMax = 3.f;
	UPROPERTY(EditAnywhere ,Category="Normal Scan")
	float ScanRadius = 1.f;
	UPROPERTY(EditAnywhere ,Category="Normal Scan")
	float ScanRadiusChangeMultiplier = 1.f;

	UFUNCTION(BlueprintCallable, Category = "Normal Scan")
	void NormalScan();
	
	UFUNCTION(BlueprintCallable, Category = "Normal Scan")
	void AdjustScanRadius(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Normal Scan")
	FVector2D GetRandomPointInsideCircle(float Radius);
	
public:
	UPROPERTY(EditAnywhere ,Category="Full Scan")
	int FullScanRayAmount = 40;
	UPROPERTY(EditAnywhere ,Category="Full Scan")
	float FullScanRate = 10.f;
	UPROPERTY(EditAnywhere ,Category="Full Scan")
	float FullScanVerticalAngle = 30.f;
	UPROPERTY(EditAnywhere ,Category="Full Scan")
	float FullScanHorizontalAngle = 30.f;

	UFUNCTION(BlueprintCallable, Category = "Full Scan")
	void StartFullScan();
	
private:
	float FullScanCurrentAngle = -FullScanVerticalAngle;
	bool FullScanInProgress;
	void UpdateFullScan(float DeltaTime);
	void PerformFullScan();
	bool GetScanDirection(float VerticalAngle, float HorizontalDegrees, FHitResult& Hit) const;

private:
	bool LineCast(const FVector& Start, const FVector& Direction, FHitResult& Hit) const;

};

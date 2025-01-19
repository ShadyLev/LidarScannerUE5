// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParticleStruct.generated.h"

USTRUCT(BlueprintType)
struct LIDARSCANNER_API FParticleStruct
{
	
	GENERATED_BODY()

public:
	// Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Struct")
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Struct")
	FLinearColor Color;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Struct")
	float Lifetime;
	
	// Constructor
	FParticleStruct()
	{
		Position = FVector::ZeroVector;
		Color = FLinearColor::White;
		Lifetime = 0.f;
	}
};

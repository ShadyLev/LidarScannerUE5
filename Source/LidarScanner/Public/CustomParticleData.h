// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CustomParticleData.generated.h"

USTRUCT(BlueprintType)
struct LIDARSCANNER_API FCustomParticleData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Struct")
	FLinearColor Color;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Struct")
	float Lifetime;
public:
	FCustomParticleData(){
		Color = FLinearColor::White;
		Lifetime = 99999.0f;
	};
};

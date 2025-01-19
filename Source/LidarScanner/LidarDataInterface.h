// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "ParticleStruct.h"
#include "LidarDataInterface.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, Category = "Lidar", meta = (DisplayName = "Lidar Data Interface"))
class LIDARSCANNER_API ULidarDataInterface : public UNiagaraDataInterface
{
	GENERATED_BODY()

	BEGIN_SHADER_PARAMETER_STRUCT(FLidarShaderParameters, )
	   SHADER_PARAMETER(int, ParticleCount)
	   SHADER_PARAMETER_SRV(Buffer<float4>, Positions)
	   SHADER_PARAMETER_SRV(Buffer<float4>, Colors)
	   SHADER_PARAMETER_SRV(Buffer<float>, Lifetimes)
	END_SHADER_PARAMETER_STRUCT()
	
public:
	virtual void PostInitProperties() override;
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;

	virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc) override;

	// DATA FUNCTIONS
	void GetParticlePosition(FVectorVMExternalFunctionContext& Context) const;
	void GetParticleColor(FVectorVMExternalFunctionContext& Context) const;
	void GetParticleLifetime(FVectorVMExternalFunctionContext& Context) const;

	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override { return Target == ENiagaraSimTarget::GPUComputeSim; }

#if WITH_EDITORONLY_DATA
	virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL) override;
	virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;
#endif	

	virtual bool UseLegacyShaderBindings() const { return false; }
	virtual void BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const override;
	virtual void SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const override;
	
	TArray<FParticleStruct> ParticleDataArray;

private:
	static const FName GetParticlePositionName;
	static const FName GetParticleColorName;
	static const FName GetParticleLifetimeName;

	static const FString ParticleCountParamName;
	static const FString LifetimeBufferName;
	static const FString ColorsBufferName;
	static const FString PositionsBufferName;
};

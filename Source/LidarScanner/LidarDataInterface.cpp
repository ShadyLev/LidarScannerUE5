// Fill out your copyright notice in the Description page of Project Settings.


#include "LidarDataInterface.h"
#include "NiagaraShaderParametersBuilder.h"

DEFINE_NDI_DIRECT_FUNC_BINDER(ULidarDataInterface, GetParticlePosition);
DEFINE_NDI_DIRECT_FUNC_BINDER(ULidarDataInterface, GetParticleColor);
DEFINE_NDI_DIRECT_FUNC_BINDER(ULidarDataInterface, GetParticleLifetime);

const FName ULidarDataInterface::GetParticlePositionName(TEXT("GetParticlePosition"));
const FName ULidarDataInterface::GetParticleColorName(TEXT("GetParticleColor"));
const FName ULidarDataInterface::GetParticleLifetimeName(TEXT("GetParticleLifetime"));

const FString ULidarDataInterface::ParticleCountParamName(TEXT("ParticleCount"));
const FString ULidarDataInterface::PositionsBufferName(TEXT("PositionsBuffer"));
const FString ULidarDataInterface::ColorsBufferName(TEXT("ColorsBuffer"));
const FString ULidarDataInterface::LifetimeBufferName(TEXT("LifetimeBuffer"));

struct FNDILidarStruct
{
	FVector Position;
	FVector4 Color;
	FFloat32 Lifetime;
};

// this proxy is used to safely copy data between game thread and render thread
struct FNDILidarProxy : public FNiagaraDataInterfaceProxy
{
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return sizeof(FParticleStruct); }

	static void ProvidePerInstanceDataForRenderThread(void* InDataForRenderThread, void* InDataFromGameThread, const FNiagaraSystemInstanceID& SystemInstance)
	{
		// initialize the render thread instance data into the pre-allocated memory
		FParticleStruct* DataForRenderThread = new (InDataForRenderThread) FParticleStruct();

		// we're just copying the game thread data, but the render thread data can be initialized to anything here and can be another struct entirely
		const FParticleStruct* DataFromGameThread = static_cast<FParticleStruct*>(InDataFromGameThread);
		*DataForRenderThread = *DataFromGameThread;
	}

	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& InstanceID) override
	{
		FParticleStruct* InstanceDataFromGT = static_cast<FParticleStruct*>(PerInstanceData);
		FParticleStruct& InstanceData = SystemInstancesToInstanceData_RT.FindOrAdd(InstanceID);
		InstanceData = *InstanceDataFromGT;

		// we call the destructor here to clean up the GT data. Without this we could be leaking memory.
		InstanceDataFromGT->~FParticleStruct();
	}

	void InitializeBuffers(int32 Count)
	{
		ParticleCount = Count;

		// Release existing buffers if already initialized
		if (PositionsBuffer.Buffer.IsValid())
		{
			PositionsBuffer.Release();
		}
		if (ColorsBuffer.Buffer.IsValid())
		{
			ColorsBuffer.Release();
		}
		if (LifetimesBuffer.Buffer.IsValid())
		{
			LifetimesBuffer.Release();
		}

		// Create and allocate GPU buffers with the required size
		FRHIResourceCreateInfo CreateInfo(TEXT("LidarBuffer"));
	}
	
	int ParticleCount;
	FRWBuffer  PositionsBuffer;
	FRWBuffer  ColorsBuffer;
	FRWBuffer  LifetimesBuffer;
	TMap<FNiagaraSystemInstanceID, FParticleStruct> SystemInstancesToInstanceData_RT;

	virtual ~FNDILidarProxy() override
	{
		PositionsBuffer.Release();
		ColorsBuffer.Release();
		LifetimesBuffer.Release();
	}
};

void ULidarDataInterface::GetFunctions(
	TArray<FNiagaraFunctionSignature>& OutFunctions)
{   
	// Retrieve particle position reading it from our splats by index
	FNiagaraFunctionSignature Sig;
	Sig.Name = GetParticlePositionName;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
		TEXT("LidarNDI")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
		TEXT("Index")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),
		TEXT("Position")));
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	OutFunctions.Add(Sig);

	FNiagaraFunctionSignature Sig2;
	Sig2.Name = GetParticleColorName;
	Sig2.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
		TEXT("LidarNDI")));
	Sig2.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
		TEXT("Index")));
	Sig2.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(),
		TEXT("Color")));
	Sig2.bMemberFunction = true;
	Sig2.bRequiresContext = false;
	OutFunctions.Add(Sig2);
	
	FNiagaraFunctionSignature Sig3;
	Sig3.Name = GetParticleLifetimeName;
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),
		TEXT("LidarNDI")));
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),
		TEXT("Index")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
		TEXT("Lifetime")));
	Sig3.bMemberFunction = true;
	Sig3.bRequiresContext = false;
	OutFunctions.Add(Sig3);
}

void ULidarDataInterface::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc)
{
	if (BindingInfo.Name == GetParticlePositionName)
	{
		OutFunc = FVMExternalFunction::CreateLambda([this](FVectorVMExternalFunctionContext& Context) { this->GetParticlePosition(Context); });
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Could not find data interface external function in %s. Received Name: %s"), *GetPathNameSafe(this), *BindingInfo.Name.ToString());
	}
	
	if (BindingInfo.Name == GetParticleColorName)
	{
		OutFunc = FVMExternalFunction::CreateLambda([this](FVectorVMExternalFunctionContext& Context) { this->GetParticleColor(Context); });
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Could not find data interface external function in %s. Received Name: %s"), *GetPathNameSafe(this), *BindingInfo.Name.ToString());
	}

	if (BindingInfo.Name == GetParticleLifetimeName)
	{
		OutFunc = FVMExternalFunction::CreateLambda([this](FVectorVMExternalFunctionContext& Context) { this->GetParticleLifetime(Context); });
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Could not find data interface external function in %s. Received Name: %s"), *GetPathNameSafe(this), *BindingInfo.Name.ToString());
	}
}

// Function defined for CPU use, understandable by Niagara
void ULidarDataInterface::GetParticlePosition(FVectorVMExternalFunctionContext& Context) const
{
	// Input is the NDI and Index of the particle
	VectorVM::FUserPtrHandler<ULidarDataInterface> InstData(Context);
	
	FNDIInputParam<int32> IndexParam(Context);
  
	// Output Position
	FNDIOutputParam<float> OutPosX(Context);
	FNDIOutputParam<float> OutPosY(Context);
	FNDIOutputParam<float> OutPosZ(Context);
	
	const auto InstancesCount = Context.GetNumInstances();

	TArray<FParticleStruct> Particles = InstData.Get()->ParticleDataArray;
	
	for(int32 i = 0; i < InstancesCount; ++i)
	{
		const int32 Index = IndexParam.GetAndAdvance();
		
		if(Particles.IsValidIndex(Index))
		{
			const auto& Particle = Particles[Index];
			OutPosX.SetAndAdvance(Particle.Position.X);
			OutPosY.SetAndAdvance(Particle.Position.Y);
			OutPosZ.SetAndAdvance(Particle.Position.Z);
		}
		else
		{
			OutPosX.SetAndAdvance(0.0f);
			OutPosY.SetAndAdvance(0.0f);
			OutPosZ.SetAndAdvance(0.0f);
		}
	}
}

void ULidarDataInterface::GetParticleColor(FVectorVMExternalFunctionContext& Context) const
{
	// Input is the NDI and Index of the particle
	VectorVM::FUserPtrHandler<ULidarDataInterface> InstData(Context);
	
	FNDIInputParam<int32> IndexParam(Context);
  
	// Output Position
	FNDIOutputParam<float> OutColorR(Context);
	FNDIOutputParam<float> OutColorG(Context);
	FNDIOutputParam<float> OutColorB(Context);
	FNDIOutputParam<float> OutColorA(Context);
	
	const auto InstancesCount = Context.GetNumInstances();

	TArray<FParticleStruct> Particles = InstData.Get()->ParticleDataArray;
	
	for(int32 i = 0; i < InstancesCount; ++i)
	{
		const int32 Index = IndexParam.GetAndAdvance();
		
		if(Particles.IsValidIndex(Index))
		{
			const auto& Particle = Particles[Index];
			OutColorR.SetAndAdvance(Particle.Color.R);
			OutColorG.SetAndAdvance(Particle.Color.G);
			OutColorB.SetAndAdvance(Particle.Color.B);
			OutColorA.SetAndAdvance(Particle.Color.A);
		}
		else
		{
			OutColorR.SetAndAdvance(0.0f);
			OutColorG.SetAndAdvance(0.0f);
			OutColorB.SetAndAdvance(0.0f);
			OutColorA.SetAndAdvance(0.0f);
		}
	}
}

void ULidarDataInterface::GetParticleLifetime(FVectorVMExternalFunctionContext& Context) const
{
	// Input is the NDI and Index of the particle
	VectorVM::FUserPtrHandler<ULidarDataInterface> InstData(Context);
	
	FNDIInputParam<int32> IndexParam(Context);
  
	// Output Position
	FNDIOutputParam<float> OutLifetime(Context);
	
	const auto InstancesCount = Context.GetNumInstances();

	TArray<FParticleStruct> Particles = InstData.Get()->ParticleDataArray;
	
	for(int32 i = 0; i < InstancesCount; ++i)
	{
		const int32 Index = IndexParam.GetAndAdvance();
		
		if(Particles.IsValidIndex(Index))
		{
			const auto& Particle = Particles[Index];
			OutLifetime.SetAndAdvance(Particle.Lifetime);
		}
		else
		{
			OutLifetime.SetAndAdvance(0.0f);
		}
	}
}

void ULidarDataInterface::SetShaderParameters(
  const FNiagaraDataInterfaceSetShaderParametersContext& Context) const
{
	// Initializing the shader parameters to be the same reference of 
	//our buffers in the proxy
	FLidarShaderParameters* ShaderParameters =
	  Context.GetParameterNestedStruct<FLidarShaderParameters>();
	if(ShaderParameters)
	{
		FNDILidarProxy& DIProxy = Context.GetProxy<FNDILidarProxy>();
		
		if(!DIProxy.PositionsBuffer.Buffer.IsValid())
		{
			// Trigger buffers initialization
			//DIProxy.InitializeBuffers(Particles.Num());
		}
		
		// Constants
		ShaderParameters->ParticleCount = DIProxy.ParticleCount;
		// Assign initialized buffers to shader parameters
		ShaderParameters->Positions = DIProxy.PositionsBuffer.SRV;
		ShaderParameters->Colors = DIProxy.ColorsBuffer.SRV;
		ShaderParameters->Lifetimes = DIProxy.LifetimesBuffer.SRV;
	}
}

bool ULidarDataInterface::GetFunctionHLSL(
  const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const
  FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, 
  int FunctionInstanceIndex, FString& OutHLSL)
{
	if(Super::GetFunctionHLSL(ParamInfo, FunctionInfo,
	  FunctionInstanceIndex, OutHLSL))
	{
		// If the function is already defined on the Super class, do not
		// duplicate its definition.
		return true;
	}
  
	if(FunctionInfo.DefinitionName == GetParticlePositionName)
	{
		static const TCHAR *FormatBounds = TEXT(R"(
      void {FunctionName}(int Index, out float3 OutPosition)
      {
        OutPosition = {PositionsBuffer}[Index].xyz;
      }
    )");
		const TMap<FString, FStringFormatArg> ArgsBounds =
		{
			{TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
			{TEXT("PositionsBuffer"),
			  FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + "PositionsBuffer")},
		   };
		OutHLSL += FString::Format(FormatBounds, ArgsBounds);
	}
	else
	{
		// Return false if the function name does not match any expected.
		return false;
	}
	
	if(FunctionInfo.DefinitionName == GetParticleColorName)
	{
		static const TCHAR *FormatBounds = TEXT(R"(
      void {FunctionName}(int Index, out float4 OutColor)
      {
        OutColor = {ColorsBuffer}[Index].xyzw;
      }
    )");
		const TMap<FString, FStringFormatArg> ArgsBounds =
		{
			{TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
			{TEXT("ColorsBuffer"),
			  FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + "ColorsBuffer")},
		   };
		OutHLSL += FString::Format(FormatBounds, ArgsBounds);
	}
	else
	{
		// Return false if the function name does not match any expected.
		return false;
	}

	if(FunctionInfo.DefinitionName == GetParticleLifetimeName)
	{
		static const TCHAR *FormatBounds = TEXT(R"(
      void {FunctionName}(int Index, out float OutLifetime)
      {
        OutLifetime = {LifetimeBuffer}[Index];
      }
    )");
		const TMap<FString, FStringFormatArg> ArgsBounds =
		{
			{TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
			{TEXT("LifetimeBuffer"),
			  FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + "LifetimeBuffer")},
		   };
		OutHLSL += FString::Format(FormatBounds, ArgsBounds);
	}
	else
	{
		// Return false if the function name does not match any expected.
		return false;
	}
	
	return true;
}

void ULidarDataInterface::GetParameterDefinitionHLSL(
  const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
	Super::GetParameterDefinitionHLSL(ParamInfo, OutHLSL);

	OutHLSL.Appendf(TEXT("int %s%s;\n"), 
	  *ParamInfo.DataInterfaceHLSLSymbol, *ParticleCountParamName);
	OutHLSL.Appendf(TEXT("Buffer<float4> %s%s;\n"),
	  *ParamInfo.DataInterfaceHLSLSymbol, *PositionsBufferName);
	OutHLSL.Appendf(TEXT("Buffer<float4> %s%s;\n"),
	  *ParamInfo.DataInterfaceHLSLSymbol, *ColorsBufferName);
	OutHLSL.Appendf(TEXT("Buffer<float> %s%s;\n"),
	  *ParamInfo.DataInterfaceHLSLSymbol, *LifetimeBufferName);
}

void ULidarDataInterface::BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const
{
	ShaderParametersBuilder.AddNestedStruct<FLidarShaderParameters>();
}

void ULidarDataInterface::PostInitProperties()
{
	Super::PostInitProperties();
	
	// Create a proxy, which we will use to pass data between CPU and GPU
	// (required to support the GPU based Niagara system).
	Proxy = MakeUnique<FNDILidarProxy>();
 
	if(HasAnyFlags(RF_ClassDefaultObject))
	{
		ENiagaraTypeRegistryFlags DIFlags =
		  ENiagaraTypeRegistryFlags::AllowAnyVariable |
		  ENiagaraTypeRegistryFlags::AllowParameter;

		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), DIFlags);
	}

	MarkRenderDataDirty();
}

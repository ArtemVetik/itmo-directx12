#include "ParticleInclude.hlsl"

cbuffer objectData : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	float aspectRatio;
};

cbuffer timeData : register(b1)
{
	float deltaTime;
	float TotalTime;
}

cbuffer particleData : register(b2)
{
	int emitCount;
	int maxParticles;
	int gridSize;
	float3 velocity;
	float3 acceleration;
	float4 startColor;
	float4 endColor;
	float lifeTime;
}

struct GS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float4 Color		: COLOR;
	float2 UV			: TEXCOORD;
};

struct ps_output
{
    float4 albedo : SV_TARGET0;
    float3 normal : SV_TARGET1;
};

ps_output main(GS_OUTPUT input) : SV_TARGET
{
    ps_output output;
	
	input.UV = input.UV * 2 - 1;

	float fade = saturate(distance(float2(0, 0), input.UV));
	float3 color = lerp(input.Color.rgb, float3(0, 0, 0), fade * fade);

    output.albedo = float4(color, 1);
    output.normal = 0;
	
    return output;
}
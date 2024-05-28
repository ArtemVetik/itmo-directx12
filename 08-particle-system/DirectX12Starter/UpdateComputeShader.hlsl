#include "ParticleInclude.hlsl"
#include "SimplexNoise.hlsl"

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

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
	float4 startColor;
	float4 endColor;
	float3 velocity;
	float lifeTime;
	float3 acceleration;
	float pad;
	int emitCount;
	int maxParticles;
	int gridSize;
}

Texture2D gDepthTexture				: register(t0);

RWStructuredBuffer<Particle> ParticlePool		: register(u0);
AppendStructuredBuffer<uint> ADeadList			: register(u1);
RWStructuredBuffer<ParticleDraw> DrawList		: register(u2);
RWStructuredBuffer<uint> DrawArgs				: register(u3);
RWStructuredBuffer<uint> DeadListCounter		: register(u4);

[numthreads(32, 1, 1)]
void main(uint id : SV_DispatchThreadID)
{
	if (id.x >= (uint)maxParticles)
		return;

	Particle particle = ParticlePool.Load(id.x);

    if (particle.Alive == 0.0f)
        return;

	particle.Age += deltaTime;
	
	particle.Alive = (float)(particle.Age < lifeTime);
	
	particle.Position += particle.Velocity * deltaTime;
	
	float3 curlPosition = particle.Position * 0.1f;
	float3 curlVelocity = curlNoise3D(curlPosition, 1.0f);
	//particle.Velocity = curlVelocity * 2;
    particle.Velocity += float3(0, -10.0f * deltaTime, 0);
	
	float3 viewPos = mul(float4(particle.Position, 1.0f), mul(world, view)).xyz;
	float4 clipSpacePos = mul(float4(viewPos, 1.0f), projection);
	clipSpacePos /= clipSpacePos.w;
	float2 aClipSpacePos = abs(clipSpacePos);
	
	if (aClipSpacePos.x < 1.0f && aClipSpacePos.y < 1.0f)
	{
		float2 uv = clipSpacePos.xy * float2(0.5f, -0.5f) + 0.5;
        float depthValue = gDepthTexture.SampleLevel(gsamPointWrap, uv, 0).r;
		
		const float n = 5.0f;
		const float f = 1000.0f;
		
		float linearEyeDepth = n * f / (depthValue * (n - f) + f);

		float radius = 0.0f;

		float surfaceThickness = 100.0f;

		if (viewPos.z > linearEyeDepth - radius)
		{
			particle.Velocity = float3(0, 50, 0);
		}
	}
	
    if ((float) (particle.Age < lifeTime / 2))
        particle.Color = float4(1, 1, 1, 1);
	else
        particle.Color = float4(1, 0, 0, 1);
	
	//// put the particle back
	ParticlePool[id.x] = particle;
	
    // newly dead?
    if (particle.Alive == 0.0f)
    {
		// Add to dead list
        ADeadList.Append(id.x);
        
        InterlockedAdd(DeadListCounter[0], 1);
    }
    else
	{
		// increment the counter on the draw list, then put the new draw data at the returned (pre-increment) index
		uint drawIndex = DrawList.IncrementCounter();

		// set up draw data
		ParticleDraw drawData;
		drawData.Index = id.x; // this particle's actual index

		DrawList[drawIndex] = drawData;
	}
}

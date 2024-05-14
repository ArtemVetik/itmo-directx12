//***************************************************************************************
// ShadowDebug.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
	float3 PosL  : POSITION;
	float2 TexC  : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD0;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);
	
	vout.TexC = vin.TexC;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	if (gShadowIndex == 0)
		return float4(gAlbedoTexture.Sample(gsamLinearWrap, pin.TexC).rgb, 1.0f);
	if (gShadowIndex == 1)
		return float4(gNormalTexture.Sample(gsamLinearWrap, pin.TexC).rgb, 1.0f);
	if (gShadowIndex == 2)
		return float4(gDepthTexture.Sample(gsamPointWrap, pin.TexC).rrr, 1.0f);
	else
		return float4(gAccumTexture.Sample(gsamPointWrap, pin.TexC).rgb, 1.0f);
}



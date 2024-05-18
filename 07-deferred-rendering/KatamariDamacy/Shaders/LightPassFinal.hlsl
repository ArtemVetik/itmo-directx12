#include "Common.hlsl"

struct VertexIn
{
	float3 PosL  : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC  : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float4 ShadowPosH : POSITION0;
    float3 PosW    : POSITION1;
    float4 PosW2    : POSITION2;
    float3 NormalW : NORMAL;
	float2 TexC  : TEXCOORD0;
	float4x4 ShadowTransform[2]  : TEXCOORD1;
};

VertexOut VS(VertexIn vIn)
{
	VertexOut vOut;
	vOut.PosH = float4(vIn.PosL, 1.0f);
	vOut.TexC = vIn.TexC;
	return vOut;
}

[earlydepthstencil]
float4 PS(VertexOut pIn) : SV_TARGET
{
	float4 hdrColor = gAccumTexture.Sample(gsamPointWrap, pIn.TexC);
	
	float gamma = 2.2;
  
    // reinhard tone mapping
    float3 mapped = hdrColor / (hdrColor + (1.0f));
    mapped = pow(mapped, (1.0 / gamma));
  
    return float4(mapped, 1.0);
}
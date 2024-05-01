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
	float4 diffuseAlbedo = gAlbedoTexture.Sample(gsamPointWrap, pIn.TexC) * gDiffuseAlbedo;
	float3 normal = gNormalTexture.Sample(gsamPointWrap, pIn.TexC).rgb;
	float4 posW = gWorldPosTexture.Sample(gsamPointWrap, pIn.TexC);
	
	float3 toEyeW = normalize(gEyePosW - posW.rgb);
	
	const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
	
	float4 directLight = ComputeLighting(gLights, mat, posW.rgb,
        normal, toEyeW, float3(posW.a, 1.0f, 1.0f));
		
	float4 ambient = gAmbientLight*diffuseAlbedo;
	float4 litColor = ambient + directLight;
	litColor.a = diffuseAlbedo.a;
	
	return litColor;
}
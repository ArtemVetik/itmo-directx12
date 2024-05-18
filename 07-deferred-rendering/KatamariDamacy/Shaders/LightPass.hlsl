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
	float4 normal = gNormalTexture.Sample(gsamPointWrap, pIn.TexC);
	
	float z = gDepthTexture.Sample(gsamPointWrap, pIn.TexC).r;
	float4 clipSpacePosition = float4(pIn.TexC * 2 - 1, z, 1);
	clipSpacePosition.y *= -1.0f;
	float4 viewSpacePosition = mul(gProjInv, clipSpacePosition);
	viewSpacePosition /= viewSpacePosition.w;
	float4 worldSpacePosition = mul(gViewInv, viewSpacePosition);
	
	float4 posW = worldSpacePosition;
	//float4 posW = gWorldPosTexture.Sample(gsamPointWrap, pIn.TexC);
	
	float3 toEyeW = normalize(gEyePosW - posW.rgb);
	
	const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
	
	float4 directLight = ComputeLighting(gLights, mat, posW.rgb,
        normal.rgb, toEyeW, float3(normal.a, 1.0f, 1.0f));
		
	float4 ambient = gAmbientLight*diffuseAlbedo;
	float4 litColor = ambient + directLight;
	litColor.a = diffuseAlbedo.a;
	
	return litColor;
}
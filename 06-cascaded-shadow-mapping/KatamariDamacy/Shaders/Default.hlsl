#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 1
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

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

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosW2 = posW;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);

	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gTexTransform).xy;

	vout.ShadowPosH = mul(vout.PosW2, gShadowTransform[0]);
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
//	if (pin.Dist < 25) return 1; else if (pin.Dist < 50) return 0.5; else return 0;
	
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinearWrap, pin.TexC) * gDiffuseAlbedo;

    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    float4 ambient = gAmbientLight*diffuseAlbedo;
	
	// Only the first light casts a shadow.
	
	float dist = length(gEyePosW - pin.PosW);
	
	uint mapIndex = 0;
	if (dist <= gCascadeDistance.r)
		mapIndex = 0;
	else if (dist <= gCascadeDistance.g)
		mapIndex = 1;
	else if (dist <= gCascadeDistance.b)
		mapIndex = 2;
	else
		mapIndex = 3;
		
	pin.ShadowPosH = mul(pin.PosW2, gShadowTransform[mapIndex]);
	
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH, mapIndex);
	//return float4(shadowFactor, 1);
    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
 
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse material.
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



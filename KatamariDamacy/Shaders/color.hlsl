Texture2D 		gDiffuseMap : register(t0);
SamplerState 	gsamLinear  : register(s0);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
    float4x4 gTexTransform;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
	float2 TexC  : TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
	float2 TexC  : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gTexTransform).xy;
    
	//vout.TexC = vin.TexC;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return gDiffuseMap.Sample(gsamLinear, pin.TexC);
}



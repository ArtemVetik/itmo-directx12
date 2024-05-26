Texture2D gAlbedoTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gAccumTexture : register(t2);
Texture2D gDepthTexture : register(t3);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD0;
};

[earlydepthstencil]
float4 main(VertexOut pIn) : SV_TARGET
{
    float4 diffuseAlbedo = gAlbedoTexture.Sample(gsamPointWrap, pIn.TexC);
    float4 normal = gNormalTexture.Sample(gsamPointWrap, pIn.TexC);
    float depth = gDepthTexture.Sample(gsamPointWrap, pIn.TexC).r;
    
    return diffuseAlbedo;
}
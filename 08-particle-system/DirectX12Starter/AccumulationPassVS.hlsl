
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

VertexOut main(VertexIn vIn)
{
	VertexOut vOut;
	vOut.PosH = float4(vIn.PosL, 1.0f);
	vOut.TexC = vIn.TexC;
	return vOut;
}
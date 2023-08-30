// Vertex shader
struct PSInput
{
	float2 texcoord : TEXCOORD0;
	float4 position : SV_POSITION;
};

PSInput VSMain(float2 position : POSITION, float2 texcoord : TEXCOORD0)
{
	PSInput result;
	// result.position = float4(position.x, position.y, 0.0f, 1.0f);
	result.position = float4(0.0f, 0.0f, 0.0f, 1.0f);
	result.texcoord = texcoord;
	return result;
}

// Pixel shader
Texture2D<float4> texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
	return texture0.Sample(sampler0, input.texcoord);
}

// The RWTexture where the shader writes the checkerboard pattern
RWTexture2D<float4> outTexture : register(u0);

[numthreads(1, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	int x = DTid.x;
	int y = DTid.y;

	// Create the checkerboard pattern
    if ((x + y) % 2 == 0)
    {
        outTexture[DTid.xy] = float4(1.0, 1.0, 1.0, 1.0);  // white
    }
    else
    {
        outTexture[DTid.xy] = float4(0.0, 0.0, 0.0, 1.0);  // black
    }
}
// Vertex shader
struct PSInput
{
    float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

PSInput VSMain(float2 position : POSITION, float2 texcoord : TEXCOORD)
{
	PSInput result;
	result.position = float4(position.x, position.y, 0.0f, 1.0f);
	result.texcoord = texcoord;
	return result;
}

// Pixel shader
Texture2D<float4> texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1.0, 0.0, 1.0, 1.0);
	// return texture0.Sample(sampler0, input.texcoord);
}

// The RWTexture where the shader writes the checkerboard pattern
RWTexture2D<float4> outTexture : register(u0);

[numthreads(1, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    // Define the size of the checkerboard squares
    int squareSize = 10;

    // Calculate which square the current pixel falls into
    int squareX = DTid.x / squareSize;
    int squareY = DTid.y / squareSize;

    // Determine the color of the square
    bool isWhite = (squareX % 2) == (squareY % 2);

    // Write the color to the output texture
    if (isWhite)
    {
        outTexture[DTid.xy] = float4(1.0, 1.0, 1.0, 1.0);  // White
    }
    else
    {
        outTexture[DTid.xy] = float4(0.0, 0.0, 0.0, 1.0);  // Black
    }
}
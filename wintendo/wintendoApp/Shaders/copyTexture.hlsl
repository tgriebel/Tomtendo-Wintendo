
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};


PSInput VSMain( float4 position : POSITION, float4 color : COLOR, float4 uv : TEXCOORD )
{
    PSInput result;

    result.position = position;
    result.uv = uv;
    result.color = color;

    return result;
}

float4 PSMain( PSInput input ) : SV_TARGET
{
   return float4( 1.0f, 1.0f, 1.0f, 1.0f );
}

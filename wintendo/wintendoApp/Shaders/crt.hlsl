//
// PUBLIC DOMAIN CRT STYLED SCAN-LINE SHADER
//
//   by Timothy Lottes
//
// This is more along the style of a really good CGA arcade monitor.
// With RGB inputs instead of NTSC.
// The shadow mask example has the mask rotated 90 degrees for less chromatic aberration.
//
// Left it unoptimized to show the theory behind the algorithm.
//
// It is an example what I personally would want as a display option for pixel art games.
// Please take and use, change, or whatever.
//
// source: https://www.shadertoy.com/view/XsjSzR
// Translated to hlsl by Thomas Griebel

struct PSInput
{
    float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

static const float VirtualSize = 2.0f;

cbuffer DisplayConstantBuffer : register(b0)
{
	float	hardScan;
	float	hardPix;
	float2	warp;
	float4	imageDim;
	float4	destImageDim;
	float	maskDark;
	float	maskLight;
	bool	enable;
};

//------------------------------------------------------------------------

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1( float c )
{
	return ( c <= 0.04045f ) ? c / 12.92f : pow( ( c + 0.055f ) / 1.055f, 2.4f );
}

float3 ToLinear( float3 c )
{
	return float3( ToLinear1( c.r ), ToLinear1( c.g ), ToLinear1( c.b ) );
}

// Linear to sRGB.
// Assuing using sRGB typed textures this should not be needed.
float ToSrgb1( float c )
{
	return( c < 0.0031308f ? c * 12.92f : 1.055f * pow( c, 0.41666f ) - 0.055f );
}

float3 ToSrgb( float3 c )
{
	return float3( ToSrgb1( c.r ), ToSrgb1( c.g ), ToSrgb1( c.b ) );
}

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
float3 Fetch( float2 pos, float2 off )
{
	const float2 virtualDim = VirtualSize * imageDim.zw;
	pos = floor( pos * virtualDim + off ) / virtualDim;
 
	if( max( abs( pos.x - 0.5f ), abs( pos.y - 0.5f ) ) > 0.5f )
		return float3( 0.0f, 0.0f, 0.0f );

	return ToLinear( g_texture.Sample( g_sampler, pos.xy ).rgb );
}

// Distance in emulated pixels to nearest texel.
float2 Dist( float2 pos )
{
	const float2 virtualDim = imageDim.zw;
	pos = pos * virtualDim;
	return -( ( pos - floor( pos ) ) - float2( 0.5f, 0.5f ) );
}

// 1D Gaussian.
float Gaus( float pos, float scale )
{
	return exp2( scale * pos * pos );
}

// 3-tap Gaussian filter along horz line.
float3 Horz3( float2 pos, float off )
{
	const float3 b = Fetch( pos, float2( -1.0f, off ) );
	const float3 c = Fetch( pos, float2( 0.0f, off ) );
	const float3 d = Fetch( pos, float2( 1.0f, off ) );
	const float dst = Dist( pos ).x;

	// Convert distance to weight.
	const float scale = hardPix;
	const float wb = Gaus( dst - 1.0f, scale );
	const float wc = Gaus( dst + 0.0f, scale );
	const float wd = Gaus( dst + 1.0f, scale );

	// Return filtered sample.
	return ( b * wb + c * wc + d * wd ) / ( wb + wc + wd );
}

// 5-tap Gaussian filter along horz line.
float3 Horz5( float2 pos, float off )
{
	const float3 a = Fetch( pos, float2( -2.0f, off ) );
	const float3 b = Fetch( pos, float2( -1.0f, off ) );
	const float3 c = Fetch( pos, float2( 0.0f, off ) );
	const float3 d = Fetch( pos, float2( 1.0f, off ) );
	const float3 e = Fetch( pos, float2( 2.0f, off ) );
	const float dst = Dist( pos ).x;

	// Convert distance to weight.
	const float scale = hardPix;
	const float wa = Gaus( dst - 2.0f, scale );
	const float wb = Gaus( dst - 1.0f, scale );
	const float wc = Gaus( dst + 0.0f, scale );
	const float wd = Gaus( dst + 1.0f, scale );
	const float we = Gaus( dst + 2.0f, scale );
  
	// Return filtered sample.
	return ( a * wa + b * wb + c * wc + d * wd + e * we ) / ( wa + wb + wc + wd + we );
}

// Return scanline weight.
float Scan( float2 pos, float off )
{
  float dst = Dist( pos ).y;
  return Gaus( dst + off, hardScan );
}

// Allow nearest three lines to effect pixel.
float3 Tri( float2 pos )
{
	const float3 a = Horz3( pos, -1.0f );
	const float3 b = Horz5( pos, 0.0f );
	const float3 c = Horz3( pos, 1.0f );
	const float wa = Scan( pos, -1.0f );
	const float wb = Scan( pos, 0.0f );
	const float wc = Scan( pos, 1.0f );
  
	return ( a * wa ) + ( b * wb ) + ( c * wc );
}

// Distortion of scanlines, and end of screen alpha.
float2 Warp( float2 pos )
{
	float2 distorted = pos * 2.0f - 1.0f;
	distorted *= float2( 1.0f + ( distorted.y * distorted.y ) * warp.x, 1.0f + ( distorted.x * distorted.x ) * warp.y );
	return ( distorted * 0.5f + 0.5f );
}

// Shadow mask.
float3 Mask( float2 pos )
{
	float3 mask = float3( maskDark, maskDark, maskDark );
	pos.x += pos.y * 3.0f;
	pos.x = frac( pos.x / 6.0f );
  
	if( pos.x < 0.333f ) 
		mask.r = maskLight;
	else if( pos.x < 0.666f )
		mask.g = maskLight;
	else
		mask.b = maskLight;

	return mask;
}

// Entry.
PSInput VSMain( float4 position : POSITION, float4 color : COLOR, float4 uv : TEXCOORD )
{
    PSInput result;

    result.position = position;
	result.color = color;
    result.uv = uv.xy;

    return result;
}

float4 PSMain( PSInput input ) : SV_TARGET
{
	if( !enable )
	{
		return g_texture.Sample( g_sampler, input.uv );
	}
	else
	{
		float4 pixel;
		
		float2 pos = Warp( input.uv );
		pixel.rgb = Tri( pos );
		//pixel.rgb *= Mask( input.position.xy );
	  
		pixel.a = 1.0f;  
		pixel.rgb = ToSrgb( pixel.rgb );

		return pixel;
	}
}

/*
	This is an example of a fragment shader
*/

// Constants
//==========

// Per-Material
//-------------

// Textures
//=========

// NOTE: These are actually not necessary to declare in Direct3D 9.
// I debated whether it would be more confusing to include the declarations or to leave them out,
// and finally decided it would be best to show them even though you're unnecessary.
// It is up to you to decide how to represent textures and samplers in your material file format.

texture2D g_color_texture;
float g_uv_modifier = 0.0;
// Samplers
//=========

sampler2D g_color_spriteSampler;

// Entry Point
//============

void main(in const float2 i_uv : TEXCOORD0,
	in const float3 i_color_perVertex : COLOR0,
	out float4 o_color : COLOR0 )
{
	// "Sample" the texture to get the color at the given texture coordinates
	float4 i_color_perVertex4 = float4(i_color_perVertex, 1.0);
	float2 uv = float2(i_uv.x + g_uv_modifier, i_uv.y);	
	float4 color_sample = tex2D(g_color_spriteSampler, uv).rgba;
	
	// The output color is the texture color
	// modified by the interpolated per-vertex color
	// and the per-material constant
	o_color = float4(color_sample * i_color_perVertex4);
	
}

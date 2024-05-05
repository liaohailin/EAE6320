/*
	This is an example of a fragment shader
*/

// Constants
//==========

// Per-Material
//-------------

uniform float3 g_colorModifier = { 1.0, 1.0, 1.0 };
uniform float3 g_light_ambient = { 0.1, 0.1, 0.1 };
uniform float3 g_light_direction = { 0, -1, 0 };
uniform float3 g_light_direction_color = { 1.0, 1.0, 1.0 };
// Textures
//=========

// NOTE: These are actually not necessary to declare in Direct3D 9.
// I debated whether it would be more confusing to include the declarations or to leave them out,
// and finally decided it would be best to show them even though you're unnecessary.
// It is up to you to decide how to represent textures and samplers in your material file format.


// Entry Point
//============

void main(in const float3 i_normal_world:NORMAL,
	in const float2 i_uv : TEXCOORD0,
	in const float3 i_color_perVertex : COLOR0,
	out float4 o_color : COLOR0 )
{
	// The output color is the texture color
	// modified by the interpolated per-vertex color
	// and the per-material constant
	o_color = float4(g_colorModifier * i_color_perVertex,
		// For now the A value should _always_ be 1.0
		1.0 );
	//o_color = float4(i_color_perVertex * g_colorModifier,
	//	// For now the A value should _always_ be 1.0
	//	1.0);
	
	float3 normal_world = normalize(i_normal_world);
	float diffuseAmount = dot(normal_world, -g_light_direction);
	diffuseAmount = saturate(diffuseAmount);
	float3 lighting_diffuse = diffuseAmount*g_light_direction_color;
	float3 color_albedo = float3(o_color.r, o_color.g, o_color.b);
	// The final lit color is the albedo modified by the lighting
	float3 color_lit = color_albedo * (lighting_diffuse + g_light_ambient);
	//o_color = float4(color_lit, 0.5);
	o_color = float4(g_colorModifier, 0.5);
	
}

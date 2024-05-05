/*
	This is an example of a vertex shader
*/

// Constants
//==========


// Entry Point
//============
void main(in const float2 i_position : POSITION, in const float3 i_color : COLOR0, in const float2 i_uv : TEXCOORD0,
	out float4 o_position : POSITION, out float3 o_color : COLOR0, out float2 o_uv : TEXCOORD0)
{
	// Calculate position
	{
		// Set the "out" position directly from the "in" position:
		o_position = float4(i_position.x, i_position.y, 0.0, 1.0);
		// Or, equivalently:
		o_position = float4(i_position.xy, 0.0, 1.0);
		o_position = float4(i_position, 0.0, 1.0);
	}

	// Calculate color
	{
		// Set the output color directly from the input color:
		o_color = i_color;
		o_uv = i_uv;
	}
}

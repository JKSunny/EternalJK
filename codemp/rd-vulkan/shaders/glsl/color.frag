#version 450

layout(location = 0) out vec4 out_color;

layout (constant_id = 4) const int color_mode = 0;

void main()
{
	switch ( color_mode ) 
	{
		default:	out_color = vec4( 1.0, 1.0, 1.0, 1.0 );		// white
		case 1:		out_color = vec4( 0.2, 1.0, 0.2, 1.0 );		// green
		case 2:		out_color = vec4( 1.0, 0.33, 0.2, 1.0 );	// red
		case 3:		out_color = vec4( 0.99, 0.66, 0.02, 1.0 );	// orange
	}
}

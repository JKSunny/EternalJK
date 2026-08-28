#version 450

layout(location = 0) out vec4 out_color;

layout (constant_id = 4) const uint color_mode = 0;

void main()
{
	switch ( color_mode ) 
	{
		case 0U:	out_color = vec4( 1.0 );		        break;  // white
		case 1U:	out_color = vec4( 0.2, 1.0, 0.2, 1.0 ); break;  // green
		case 2U:	out_color = vec4( 1.0, 0.33, 0.2, 1.0 );break;  // red
        default: // custom
            out_color = vec4(
                float((color_mode >> 24) & 0xffu),
                float((color_mode >> 16) & 0xffu),
                float((color_mode >>  8) & 0xffu),
                float( color_mode        & 0xffu)
            ) / 255.0;
            break;
	}
}

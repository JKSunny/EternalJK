// JKGalaxies Lightsaber Blade Graphics
// --------------------------------------------------------

// Saber contact flare, note that this is currently bigger than the BaseJKA one. 

gfx/lightsabers/contactFlare
{
	nomipmaps
    {
        map gfx/lightsabers/contactFlare
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

// JKG customizable RGBa - default lightsaber blades

gfx/lightsabers/ignitionFlare
{
	cull	twosided
    {
        map gfx/lightsabers/ignitionFlare
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
    {
        map gfx/lightsabers/ignitionFlare_core
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

gfx/lightsabers/rgb_glow
{
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/rgb_glow_static
{
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/rgb_blade_sharp
{
	cull	twosided
    {
        map gfx/lightsabers/blade_sharp
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgb_blade_dull
{
	cull	twosided
    {
        map gfx/lightsabers/blade_dull
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgb_blade_cheap
{
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgb_blade_cheap_static
{
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap_static
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgb_blade_compressed
{
	qer_editorimage	gfx/lightsabers/blade_compressed
	cull	twosided
    {
        animMap 5.2 gfx/lightsabers/blade_compressed gfx/lightsabers/blade_compressed2 gfx/lightsabers/blade_compressed3 gfx/lightsabers/blade_compressed4
        blendFunc GL_ONE GL_ONE
        rgbGen wave inversesawtooth 0 1 0 5.2
    }
    {
        animMap 5.2 gfx/lightsabers/blade_compressed4 gfx/lightsabers/blade_compressed gfx/lightsabers/blade_compressed2 gfx/lightsabers/blade_compressed3
        blendFunc GL_ONE GL_ONE
        rgbGen wave sawtooth 0 1 0 5.2
    }
}

gfx/lightsabers/rgb_blade_unstable
{
	qer_editorimage	gfx/lightsabers/blade_unstable
	cull	twosided
    {
        animMap 5.2 gfx/lightsabers/blade_unstable gfx/lightsabers/blade_unstable2 gfx/lightsabers/blade_unstable3 gfx/lightsabers/blade_unstable4
        blendFunc GL_ONE GL_ONE
        rgbGen wave inversesawtooth 0 0.5 0 5.2
    }
    {
        animMap 5.2 gfx/lightsabers/blade_unstable4 gfx/lightsabers/blade_unstable gfx/lightsabers/blade_unstable2 gfx/lightsabers/blade_unstable3
        blendFunc GL_ONE GL_ONE
        rgbGen wave sawtooth 0 0.5 0 5.2
    }
}

gfx/lightsabers/rgb_trail
{
	cull	twosided
    {
        clampmap gfx/lightsabers/trail_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
    {
        clampmap gfx/lightsabers/trail_core
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

gfx/lightsabers/rgb_trail_unstable
{
	cull	twosided
    {
        clampmap gfx/lightsabers/unstable_trail_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
    {
        clampmap gfx/lightsabers/unstable_trail_core
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

// RGBalpha allows for dark elements in saber blades and trails, created for rare color variations. 

gfx/lightsabers/ignitionFlare_alpha
{
	qer_editorimage	gfx/lightsabers/ignitionFlare
	cull	twosided
    {
        map gfx/lightsabers/ignitionFlare
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        map gfx/lightsabers/ignitionFlare
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
    {
        map gfx/lightsabers/ignitionFlare_core
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

gfx/lightsabers/rgbalpha_glow
{
	qer_editorimage	gfx/lightsabers/glow
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/rgbalpha_glow_static
{
	qer_editorimage	gfx/lightsabers/glow_static
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/rgbalpha_blade_sharp
{
	qer_editorimage	gfx/lightsabers/blade_sharp
	cull	twosided
    {
        map gfx/lightsabers/blade_sharp
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        map gfx/lightsabers/blade_sharp
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgbalpha_blade_dull
{
	qer_editorimage	gfx/lightsabers/blade_dull
	cull	twosided
    {
        map gfx/lightsabers/blade_dull
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        map gfx/lightsabers/blade_dull
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgbalpha_blade_cheap
{
	qer_editorimage	gfx/lightsabers/blade_cheap
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        map gfx/lightsabers/blade_cheap
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgbalpha_blade_cheap_static
{
	qer_editorimage	gfx/lightsabers/blade_cheap_static
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap_static
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        map gfx/lightsabers/blade_cheap_static
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/rgbalpha_blade_compressed
{
	qer_editorimage	gfx/lightsabers/blade_compressed
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        animMap 5.2 gfx/lightsabers/blade_compressed gfx/lightsabers/blade_compressed2 gfx/lightsabers/blade_compressed3 gfx/lightsabers/blade_compressed4
        blendFunc GL_ONE GL_ONE
        rgbGen wave inversesawtooth 0 1 0 5.2
    }
    {
        animMap 5.2 gfx/lightsabers/blade_compressed4 gfx/lightsabers/blade_compressed gfx/lightsabers/blade_compressed2 gfx/lightsabers/blade_compressed3
        blendFunc GL_ONE GL_ONE
        rgbGen wave sawtooth 0 1 0 5.2
    }
}

gfx/lightsabers/rgbalpha_blade_unstable
{
	qer_editorimage	gfx/lightsabers/blade_unstable
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        animMap 5.2 gfx/lightsabers/blade_unstable gfx/lightsabers/blade_unstable2 gfx/lightsabers/blade_unstable3 gfx/lightsabers/blade_unstable4
        blendFunc GL_ONE GL_ONE
        rgbGen wave inversesawtooth 0 0.5 0 5.2
    }
    {
        animMap 5.2 gfx/lightsabers/blade_unstable4 gfx/lightsabers/blade_unstable gfx/lightsabers/blade_unstable2 gfx/lightsabers/blade_unstable3
        blendFunc GL_ONE GL_ONE
        rgbGen wave sawtooth 0 0.5 0 5.2
    }
}

gfx/lightsabers/rgbalpha_trail
{
	cull	twosided
    {
        clampmap gfx/lightsabers/trail_glow
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        clampmap gfx/lightsabers/trail_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
    {
        clampmap gfx/lightsabers/trail_core
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

gfx/lightsabers/rgbalpha_trail_unstable
{
	cull	twosided
    {
        clampmap gfx/lightsabers/unstable_trail_glow
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
    {
        clampmap gfx/lightsabers/unstable_trail_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
    {
        clampmap gfx/lightsabers/unstable_trail_core
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}

// Common color presets, with RGB values

gfx/lightsabers/red_glow
{
	qer_editorimage	gfx/lightsabers/glow
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.674510 0.082353 0.015686 )
    }
}

gfx/lightsabers/red_glow_static
{
	qer_editorimage	gfx/lightsabers/glow_static
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.674510 0.082353 0.015686 )
    }
}

gfx/lightsabers/orange_glow
{
	qer_editorimage	gfx/lightsabers/glow
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.780392 0.360784 0.113725 )
    }
}

gfx/lightsabers/orange_glow_static
{
	qer_editorimage	gfx/lightsabers/glow_static
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.780392 0.360784 0.113725 )
    }
}

gfx/lightsabers/yellow_glow
{
	qer_editorimage	gfx/lightsabers/glow
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.690196 0.513726 0.101961 )
    }
}

gfx/lightsabers/yellow_glow_static
{
	qer_editorimage	gfx/lightsabers/glow_static
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.690196 0.513726 0.101961 )
    }
}

gfx/lightsabers/green_glow
{
	qer_editorimage	gfx/lightsabers/glow
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.294118 0.580392 0.105882 )
    }
}

gfx/lightsabers/green_glow_static
{
	qer_editorimage	gfx/lightsabers/glow_static
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.294118 0.580392 0.105882 )
    }
}

gfx/lightsabers/blue_glow
{
	qer_editorimage	gfx/lightsabers/glow
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.113725 0.435294 0.588235 )
    }
}

gfx/lightsabers/blue_glow_static
{
	qer_editorimage	gfx/lightsabers/glow_static
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.113725 0.435294 0.588235 )
    }
}

gfx/lightsabers/purple_glow
{
	qer_editorimage	gfx/lightsabers/glow
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.454902 0.101961 0.690196 )
    }
}

gfx/lightsabers/purple_glow_static
{
	qer_editorimage	gfx/lightsabers/glow_static
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen const ( 0.454902 0.101961 0.690196 )
    }
}

// Pure white - blade lines are used in preset colors too (only the glow shaders determine color)
// Note that for trails we are supposed to use the shaders found under RGBa. Only exception is black, that wouldn't look very good. 

gfx/lightsabers/white_glow
{
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/white_glow_static
{
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/white_blade_sharp
{
	cull	twosided
    {
        map gfx/lightsabers/blade_sharp
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/white_blade_dull
{
	cull	twosided
    {
        map gfx/lightsabers/blade_dull
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/white_blade_cheap
{
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/lightsabers/white_blade_cheap_static
{
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap_static
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

// Inverted/Black/Shadow blades - completely devoid of color and white shades... Perfect Dark. 

gfx/lightsabers/ignitionFlare_black
{
	qer_editorimage	gfx/lightsabers/ignitionFlare
	cull	twosided
    {
        map gfx/lightsabers/ignitionFlare
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen vertex
    }
    {
        map gfx/lightsabers/ignitionFlare_core
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
}

gfx/lightsabers/black_glow
{
	cull	twosided
    {
        map gfx/lightsabers/glow
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/black_glow_static
{
	cull	twosided
    {
        map gfx/lightsabers/glow_static
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        glow
        rgbGen vertex
    }
}

gfx/lightsabers/black_blade_sharp
{
	cull	twosided
    {
        map gfx/lightsabers/blade_sharp
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen vertex
    }
}

gfx/lightsabers/black_blade_dull
{
	cull	twosided
    {
        map gfx/lightsabers/blade_dull
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen vertex
    }
}

gfx/lightsabers/black_blade_cheap
{
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen vertex
    }
}

gfx/lightsabers/black_blade_cheap_static
{
	cull	twosided
    {
        map gfx/lightsabers/blade_cheap_static
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen vertex
    }
}

// Shadow trail - black exclusive

gfx/lightsabers/black_trail
{
	cull	twosided
    {
        clampmap gfx/lightsabers/trail_glow
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        glow
        rgbGen vertex
    }
    {
        clampmap gfx/lightsabers/trail_core
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen identity
    }
}

// Last modified by: BlasTech on 2010-10-18
// JKGalaxies Photosourced Animating Flames by BlasTech

gfx/flames/fire
{
	qer_editorimage	gfx/flames/fire10
	cull	twosided
    {
        animMap 20 gfx/flames/fire01 gfx/flames/fire02 gfx/flames/fire03 gfx/flames/fire04 gfx/flames/fire05 gfx/flames/fire06 gfx/flames/fire07 gfx/flames/fire08 gfx/flames/fire09 gfx/flames/fire10 gfx/flames/fire11 gfx/flames/fire12 gfx/flames/fire13 gfx/flames/fire14 gfx/flames/fire15 gfx/flames/fire16 gfx/flames/fire17 gfx/flames/fire18 gfx/flames/fire19 gfx/flames/fire20 
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

// Interpolated, does not work with alpha fade or RGB generators. 

gfx/flames/fire_smooth
{
	qer_editorimage	gfx/flames/fire05
	cull	twosided
    {
        animMap 20 gfx/flames/fire01 gfx/flames/fire02 gfx/flames/fire03 gfx/flames/fire04 gfx/flames/fire05 gfx/flames/fire06 gfx/flames/fire07 gfx/flames/fire08 gfx/flames/fire09 gfx/flames/fire10 gfx/flames/fire11 gfx/flames/fire12 gfx/flames/fire13 gfx/flames/fire14 gfx/flames/fire15 gfx/flames/fire16 gfx/flames/fire17 gfx/flames/fire18 gfx/flames/fire19 gfx/flames/fire20 
        blendFunc GL_ONE GL_ONE
        rgbGen wave inversesawtooth 0 0.6 0 20
    }
    {
        animMap 20 gfx/flames/fire02 gfx/flames/fire03 gfx/flames/fire04 gfx/flames/fire05 gfx/flames/fire06 gfx/flames/fire07 gfx/flames/fire08 gfx/flames/fire09 gfx/flames/fire10 gfx/flames/fire11 gfx/flames/fire12 gfx/flames/fire13 gfx/flames/fire14 gfx/flames/fire15 gfx/flames/fire16 gfx/flames/fire17 gfx/flames/fire18 gfx/flames/fire19 gfx/flames/fire20 gfx/flames/fire01 
        blendFunc GL_ONE GL_ONE
        rgbGen wave sawtooth 0 0.6 0 20
    }
}

// Interpolated flame split up in its two components, allows for RGB in effects (colored fires on Siege Korriban, blue torches in CTF Rift). 

gfx/flames/fire01
{
	qer_editorimage	gfx/flames/fire01
	cull	twosided
    {
        animMap 20 gfx/flames/fire01 gfx/flames/fire02 gfx/flames/fire03 gfx/flames/fire04 gfx/flames/fire05 gfx/flames/fire06 gfx/flames/fire07 gfx/flames/fire08 gfx/flames/fire09 gfx/flames/fire10 gfx/flames/fire11 gfx/flames/fire12 gfx/flames/fire13 gfx/flames/fire14 gfx/flames/fire15 gfx/flames/fire16 gfx/flames/fire17 gfx/flames/fire18 gfx/flames/fire19 gfx/flames/fire20 
        blendFunc GL_SRC_ALPHA GL_ONE
        rgbGen vertex
        alphaGen wave inversesawtooth 0 1 0 20
    }
}

gfx/flames/fire02
{
	qer_editorimage	gfx/flames/fire02
	cull	twosided
    {
        animMap 20 gfx/flames/fire02 gfx/flames/fire03 gfx/flames/fire04 gfx/flames/fire05 gfx/flames/fire06 gfx/flames/fire07 gfx/flames/fire08 gfx/flames/fire09 gfx/flames/fire10 gfx/flames/fire11 gfx/flames/fire12 gfx/flames/fire13 gfx/flames/fire14 gfx/flames/fire15 gfx/flames/fire16 gfx/flames/fire17 gfx/flames/fire18 gfx/flames/fire19 gfx/flames/fire20 gfx/flames/fire01 
        blendFunc GL_SRC_ALPHA GL_ONE
        rgbGen vertex
        alphaGen wave sawtooth 0 1 0 20
    }
}

// Fiery explosion debris

gfx/flames/fire_debris
{
	qer_editorimage	gfx/flames/fire10
	cull	twosided
    {
        map gfx/flames/fire_debris
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
    {
        animMap 30 gfx/flames/fire01 gfx/flames/fire02 gfx/flames/fire03 gfx/flames/fire04 gfx/flames/fire05 gfx/flames/fire06 gfx/flames/fire07 gfx/flames/fire08 gfx/flames/fire09 gfx/flames/fire10 gfx/flames/fire11 gfx/flames/fire12 gfx/flames/fire13 gfx/flames/fire14 gfx/flames/fire15 gfx/flames/fire16 gfx/flames/fire17 gfx/flames/fire18 gfx/flames/fire19 gfx/flames/fire20 
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

// Fire Particles

gfx/flames/particle_fire1
{
	cull	twosided
    {
        map gfx/flames/particle_fire1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue1
{
	cull	twosided
    {
        map gfx/flames/particle_blue1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white1
{
	cull	twosided
    {
        map gfx/flames/particle_white1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur1
{
	cull	twosided
    {
        map gfx/flames/particle_blur1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_fire2
{
	cull	twosided
    {
        map gfx/flames/particle_fire2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue2
{
	cull	twosided
    {
        map gfx/flames/particle_blue2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white2
{
	cull	twosided
    {
        map gfx/flames/particle_white2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur2
{
	cull	twosided
    {
        map gfx/flames/particle_blur2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_fire3
{
	cull	twosided
    {
        map gfx/flames/particle_fire3
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue3
{
	cull	twosided
    {
        map gfx/flames/particle_blue3
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white3
{
	cull	twosided
    {
        map gfx/flames/particle_white3
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur3
{
	cull	twosided
    {
        map gfx/flames/particle_blur3
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_fire4
{
	cull	twosided
    {
        map gfx/flames/particle_fire4
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue4
{
	cull	twosided
    {
        map gfx/flames/particle_blue4
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white4
{
	cull	twosided
    {
        map gfx/flames/particle_white4
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur4
{
	cull	twosided
    {
        map gfx/flames/particle_blur4
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_fire5
{
	cull	twosided
    {
        map gfx/flames/particle_fire5
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue5
{
	cull	twosided
    {
        map gfx/flames/particle_blue5
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white5
{
	cull	twosided
    {
        map gfx/flames/particle_white5
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur5
{
	cull	twosided
    {
        map gfx/flames/particle_blur5
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_fire6
{
	cull	twosided
    {
        map gfx/flames/particle_fire6
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue6
{
	cull	twosided
    {
        map gfx/flames/particle_blue6
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white6
{
	cull	twosided
    {
        map gfx/flames/particle_white6
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur6
{
	cull	twosided
    {
        map gfx/flames/particle_blur6
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_fire7
{
	cull	twosided
    {
        map gfx/flames/particle_fire7
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue7
{
	cull	twosided
    {
        map gfx/flames/particle_blue7
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white7
{
	cull	twosided
    {
        map gfx/flames/particle_white7
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur7
{
	cull	twosided
    {
        map gfx/flames/particle_blur7
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_fire8
{
	cull	twosided
    {
        map gfx/flames/particle_fire8
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blue8
{
	cull	twosided
    {
        map gfx/flames/particle_blue8
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_white8
{
	cull	twosided
    {
        map gfx/flames/particle_white8
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/flames/particle_blur8
{
	cull	twosided
    {
        map gfx/flames/particle_blur8
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

// Smoke Trails

gfx/flames/smokeTrail
{
	cull	twosided
    {
        map gfx/flames/smokeTrail
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale 1 0.7
        tcMod scroll 0 -0.2
    }
    {
        map gfx/flames/smokeTrail
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale 1 0.3
        tcMod scroll 0 -0.3
    }
}

gfx/flames/smokeTrail2
{
	cull	twosided
    {
        map gfx/flames/smokeTrail2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/flames/smokeTrail3
{
	cull	twosided
    {
        map gfx/flames/smokeTrail3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/flames/smokeTrail_alpha
{
	qer_editorimage	gfx/flames/smokeTrail_alpha
	surfaceparm	trans
	q3map_onlyvertexlighting
	cull	twosided
    {
        map gfx/flames/smokeTrail_alpha
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        tcMod scale 1 0.7
        tcMod scroll 0 -0.2
    }
    {
        map gfx/flames/smokeTrail_alpha
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        tcMod scale 1 0.3
        tcMod scroll 0 -0.3
    }
}

gfx/flames/smokeTrail_alpha2
{
	qer_editorimage	gfx/flames/smokeTrail_alpha2
	surfaceparm	trans
	q3map_onlyvertexlighting
	cull	twosided
    {
        map gfx/flames/smokeTrail_alpha2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/flames/smokeTrail_alpha3
{
	qer_editorimage	gfx/flames/smokeTrail_alpha3
	surfaceparm	trans
	q3map_onlyvertexlighting
	cull	twosided
    {
        map gfx/flames/smokeTrail_alpha3
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

// Inverted, for shadows

gfx/flames/smokeTrail2_black
{
	cull	twosided
    {
        map gfx/flames/smokeTrail2
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen vertex
    }
}

gfx/flames/smokeTrail3_black
{
	cull	twosided
    {
        map gfx/flames/smokeTrail3
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        rgbGen vertex
    }
}

// Generic tiny sparks

gfx/flames/spark1
{
	cull	twosided
    {
        map gfx/flames/spark1
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/flames/spark2
{
	cull	twosided
    {
        map gfx/flames/spark2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/flames/spark3
{
	cull	twosided
    {
        map gfx/flames/spark3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/flames/spark4
{
	cull	twosided
    {
        map gfx/flames/spark4
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/flames/spark5
{
	cull	twosided
    {
        map gfx/flames/spark5
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

// Last modified by: BlasTech on 2011-11-06
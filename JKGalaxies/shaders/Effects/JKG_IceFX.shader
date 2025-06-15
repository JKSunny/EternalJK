// JKGalaxies Ice EFX Shaders by BlasTech, currently only used by CryoBan Grenade EFX. 

gfx/burnmarks/freeze
{
	qer_editorimage	gfx/burnmarks/freeze
	q3map_material	Ice
	polygonOffset
	q3map_nolightmap
	sort	decal
    {
        map gfx/burnmarks/freeze
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/burnmarks/freeze2
{
	qer_editorimage	gfx/burnmarks/freeze2
	q3map_material	Ice
	polygonOffset
	q3map_nolightmap
	sort	decal
    {
        map gfx/burnmarks/freeze2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/atmospheric/snowflake
{
	qer_editorimage	gfx/atmospheric/snowflake_32
	polygonOffset
	q3map_nolightmap
	sort	decal
    {
        map gfx/atmospheric/snowflake_32
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/effects/snowflake1
{
	qer_editorimage	gfx/atmospheric/snowflake_32
	polygonOffset
	q3map_nolightmap
	sort	decal
    {
        map gfx/atmospheric/snowflake_32
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

models/effects/ice
{
	q3map_material	Ice
	q3map_nolightmap
	cull	twosided
    {
        map gfx/playeroverlays/ice
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
//        rgbGen lightingDiffuse
    }
    {
        map envmap_spec
        blendFunc GL_DST_COLOR GL_ONE
//        glow
        detail
        rgbGen lightingDiffuse
//        alphaGen lightingSpecular
        tcGen environment
    }
}

models/effects/ice_melting
{
	q3map_material	Ice
	q3map_nolightmap
	deformvertexes	wave	4 sin 0 0.06 0.3 1.6
	cull	twosided
    {
        map gfx/playeroverlays/ice
        blendFunc GL_SRC_ALPHA GL_SRC_COLOR
        rgbGen identity
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        detail
        rgbGen lightingDiffuse
        alphaGen lightingSpecular
        tcGen environment
    }
}

models/effects/ice_old
{
	q3map_material	Ice
	q3map_nolightmap
	cull	twosided
    {
        map models/effects/ice
        blendFunc GL_SRC_ALPHA GL_SRC_COLOR
        rgbGen lightingDiffuse
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        detail
        rgbGen lightingDiffuse
        alphaGen lightingSpecular
        tcGen environment
    }
}

// Last modified by: Silverfang on 2025-06-14
// Previously modified by: BlasTech on 2010-11-22
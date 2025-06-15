// JKGalaxies Alpha Overlay Blood Shaders by BlasTech. 

// IMPORTANT REGARDING ALPHA OVERLAY SHADERS: New textures -must- be clone brushed in GIMP from a new transparent document (= black background in simple shell extension thumbnail viewers like .TGA Thumbplug or XnView) in order to work with this format. Also, don't overdo these in effects as they multiply their values. They are preferably made in grayscale when used on top of each other. RGB modifers don't work on these, and alpha modifers generally look bad. 

gfx/blood/Blood_DropsBig
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_dropsbig
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_SplatBig
{
	polygonOffset
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_splatbig
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_DropsMed
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_dropsmed
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_SplatMed
{
	polygonOffset
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_splatmed
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_DropsSmall
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_dropssmall
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_SplatSmall
{
	polygonOffset
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_splatsmall
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_DropsTiny
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_dropstiny
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_SplatTiny
{
	polygonOffset
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_splattiny
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_SplatPool
{
	polygonOffset
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_splatpool
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_DropsPool
{
	polygonOffset
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_dropspool
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

// To be drawn on G2 models by blades. Not preferred for lightsabers. 

gfx/blood/Blood_Slashmark
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_slashmark
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/blood/Blood_Slashmark_Burnt
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/blood/blood_slashmark
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
    {
        map gfx/burnmarks/burnline
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

// Last modified by: Silverfang on 2025-06-14
// Previously modified by: BlasTech on 2010-08-18
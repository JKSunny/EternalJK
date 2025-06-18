// JKGalaxies Water by BlasTech. Most effects are done with overlay blending, please refer to JKG_Blood.shader for further information. 

// BaseJKA wake around player - now with special blending!

wake_jkg
{
	q3map_nolightmap
	cull	twosided
    {
        clampmap gfx/water/overlay_ripples
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
        tcMod rotate 1
        tcMod stretch sin 1 0.03 0 0.7
    }
}

// Various versions of the water GFX textures

gfx/water/alpha_bubbles
{
	qer_editorimage	gfx/water/alpha_bubbles
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_bubbles
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/alpha_bubbles2
{
	qer_editorimage	gfx/water/alpha_bubbles2
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_bubbles2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/alpha_bubbles3
{
	qer_editorimage	gfx/water/alpha_bubbles3
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_bubbles3
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/overlay_bubbles
{
	qer_editorimage	gfx/water/overlay_bubbles
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_bubbles
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_bubbles2
{
	qer_editorimage	gfx/water/overlay_bubbles2
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_bubbles2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_bubbles3
{
	qer_editorimage	gfx/water/overlay_bubbles3
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_bubbles3
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_bubbles4
{
	qer_editorimage	gfx/water/alpha_bubbles
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_bubbles
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/overlay_bubbles5
{
	qer_editorimage	gfx/water/alpha_bubbles
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_bubbles
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/overlay_bubbles6
{
	qer_editorimage	gfx/water/alpha_bubbles
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_bubbles
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/screen_bubbles
{
	cull	twosided
    {
        map gfx/water/screen_bubbles
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_bubbles2
{
	cull	twosided
    {
        map gfx/water/screen_bubbles2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_bubbles3
{
	cull	twosided
    {
        map gfx/water/screen_bubbles3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_bubbles_glow
{
	cull	twosided
    {
        map gfx/water/screen_bubbles
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/screen_bubbles2_glow
{
	cull	twosided
    {
        map gfx/water/screen_bubbles2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/screen_bubbles3_glow
{
	cull	twosided
    {
        map gfx/water/screen_bubbles3
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/alpha_woosh
{
	qer_editorimage	gfx/water/alpha_woosh
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_woosh
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/alpha_woosh2
{
	qer_editorimage	gfx/water/alpha_woosh2
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_woosh2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/alpha_woosh3
{
	qer_editorimage	gfx/water/alpha_woosh3
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_woosh3
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

// Decals used by the carbonite rifle - note that they're darkened here in the shader.

gfx/water/alpha_woosh_decal
{
	qer_editorimage	gfx/water/alpha_woosh
	polygonOffset
	q3map_nolightmap
	sort	decal
	cull	twosided
    {
        map gfx/water/alpha_woosh
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen const ( 0.3 0.3 0.3 )
        alphaGen vertex
    }
}

gfx/water/alpha_woosh2_decal
{
	qer_editorimage	gfx/water/alpha_woosh2
	polygonOffset
	q3map_nolightmap
	sort	decal
	cull	twosided
    {
        map gfx/water/alpha_woosh2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen const ( 0.3 0.3 0.3 )
        alphaGen vertex
    }
}

gfx/water/alpha_woosh3_decal
{
	qer_editorimage	gfx/water/alpha_woosh3
	polygonOffset
	q3map_nolightmap
	sort	decal
	cull	twosided
    {
        map gfx/water/alpha_woosh3
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen const ( 0.3 0.3 0.3 )
        alphaGen vertex
    }
}

gfx/water/alphaoverlay_woosh
{
	qer_editorimage	gfx/water/alpha_woosh
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_woosh
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/alphaoverlay_woosh2
{
	qer_editorimage	gfx/water/alpha_woosh2
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_woosh2
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/alphaoverlay_woosh3
{
	qer_editorimage	gfx/water/alpha_woosh3
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/alpha_woosh3
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
    }
}

gfx/water/overlay_woosh
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_woosh
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_woosh2
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_woosh2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_woosh3
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_woosh3
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_ripple
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_ripple
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_ripple2
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_ripple2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_ripple3
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_ripple3
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_ripple4
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_ripple4
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_ripple5
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_ripple5
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/overlay_ripples
{
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/overlay_ripples
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/water/screen_ripple
{
	cull	twosided
    {
        map gfx/water/screen_ripple
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_ripple2
{
	cull	twosided
    {
        map gfx/water/screen_ripple2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_ripple3
{
	cull	twosided
    {
        map gfx/water/screen_ripple3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_ripple4
{
	cull	twosided
    {
        map gfx/water/screen_ripple4
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_ripple5
{
	cull	twosided
    {
        map gfx/water/screen_ripple5
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_ripples
{
	cull	twosided
    {
        map gfx/water/screen_ripples
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/water/screen_ripple_glow
{
	qer_editorimage	gfx/water/screen_ripple
	cull	twosided
    {
        map gfx/water/screen_ripple
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/screen_ripple2_glow
{
	qer_editorimage	gfx/water/screen_ripple2
	cull	twosided
    {
        map gfx/water/screen_ripple2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/screen_ripple3_glow
{
	qer_editorimage	gfx/water/screen_ripple3
	cull	twosided
    {
        map gfx/water/screen_ripple3
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/screen_ripple4_glow
{
	qer_editorimage	gfx/water/screen_ripple4
	cull	twosided
    {
        map gfx/water/screen_ripple4
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/screen_ripple5_glow
{
	qer_editorimage	gfx/water/screen_ripple5
	cull	twosided
    {
        map gfx/water/screen_ripple5
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/water/screen_ripples_glow
{
	qer_editorimage	gfx/water/screen_ripples
	cull	twosided
    {
        map gfx/water/screen_ripples
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/world/rain
{
	cull	twosided
    {
        clampMap gfx/world/rain
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen vertex
    }
}


// Stargate Shaders
/// Incoming wormhole's event horizon light beam

textures/amace_vault/lightbeam
{
	qer_editorimage	gfx/flares/lightbeam_pillar
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	trans
	q3map_nodlight
	q3map_nolightmap
	cull	twosided
    {
        map gfx/flares/lightbeam_pillar
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen identity
    }
}

// The Event Horizon itself, used in both incoming and outgoing wormholes. 

textures/aMace_Vault/sg_eventhorizon
{
	qer_editorimage	textures/aMace_Vault/sg_eh1
	q3map_tesssize	32
	surfaceparm	nonsolid
	q3map_material	Water
	q3map_nolightmap
	cull	twosided
	deformvertexes	wave	16 sin 0 2.4 0 0.8
    {
        animMap 0.4 textures/aMace_Vault/sg_eh1 textures/aMace_Vault/sg_eh2 textures/aMace_Vault/sg_eh3 textures/aMace_Vault/sg_eh4
        rgbGen wave inversesawtooth 0 1 0 0.4
        tcMod stretch sin 1 0.004 0 1.2
    }
    {
        animMap 0.4 textures/aMace_Vault/sg_eh2 textures/aMace_Vault/sg_eh3 textures/aMace_Vault/sg_eh4 textures/aMace_Vault/sg_eh1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen wave sawtooth 0 1 0 0.4
        tcMod stretch sin 1 0.006 0 1.6
    }
    {
        map envmap
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen const 0.35//lightingSpecular
        tcGen environment
    }
    {
        animMap 0.8 textures/aMace_Vault/sg_eh1_glow textures/aMace_Vault/sg_eh2_glow textures/aMace_Vault/sg_eh3_glow textures/aMace_Vault/sg_eh4_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen wave inversesawtooth 0 1 0 0.8
        tcMod stretch sin 1 0.004 0 1.1
    }
    {
        animMap 0.8 textures/aMace_Vault/sg_eh2_glow textures/aMace_Vault/sg_eh3_glow textures/aMace_Vault/sg_eh4_glow textures/aMace_Vault/sg_eh1_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen wave sawtooth 0 1 0 0.8
        tcMod stretch sin 1 0.006 0 1.3
    }
}

// Event horizon glow maps for EFX and caustics

textures/aMace_Vault/sg_eh1_glow2
{
	cull	twosided
    {
        map textures/aMace_Vault/sg_eh1_glow2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

textures/aMace_Vault/sg_eh2_glow2
{
	cull	twosided
    {
        map textures/aMace_Vault/sg_eh2_glow2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

textures/aMace_Vault/sg_eh3_glow2
{
	cull	twosided
    {
        map textures/aMace_Vault/sg_eh3_glow2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

textures/aMace_Vault/sg_eh4_glow2
{
	cull	twosided
    {
        map textures/aMace_Vault/sg_eh4_glow2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

textures/aMace_Vault/sg_eventhorizon_caustics
{
	qer_editorimage	textures/aMace_Vault/sg_eh1_glow2
	qer_trans	0.6
	polygonOffset
	surfaceparm	nonsolid
	q3map_nolightmap
	sort	decal
    {
        animMap 0.4 textures/aMace_Vault/sg_eh1_glow2 textures/aMace_Vault/sg_eh2_glow2 textures/aMace_Vault/sg_eh3_glow2 textures/aMace_Vault/sg_eh4_glow2
        blendFunc GL_SRC_ALPHA GL_ONE
        rgbGen wave inversesawtooth 0 0.2 0 0.4
        alphaGen lightingSpecular
        tcMod stretch sin 1 0.004 0 0.7
    }
    {
        animMap 0.4 textures/aMace_Vault/sg_eh2_glow2 textures/aMace_Vault/sg_eh3_glow2 textures/aMace_Vault/sg_eh4_glow2 textures/aMace_Vault/sg_eh1_glow2
        blendFunc GL_SRC_ALPHA GL_ONE
        rgbGen wave sawtooth 0 0.2 0 0.4
        alphaGen lightingSpecular
        tcMod stretch sin 1 0.006 0 0.8
    }
}

// Backup of old outdated shader

textures/amace_vault/sg_eventhorizon_old
{
	qer_editorimage	textures/amace_Vault/sg_eventhorizon
	q3map_tesssize	32
	qer_trans	0.6
	surfaceparm	nonsolid
//	surfaceparm	nonopaque
//	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	cull	twosided
	deformvertexes	wave	16 sin 0 2.4 0 0.7
    {
        map textures/amace_vault/sg_eventhorizon
        tcGen environment
        tcMod rotate -40
    }
    {
        map gfx/water/surface
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen identity
        tcMod scroll 0.02 -0.05
        tcMod turb 1 0.01 0 0.3
    }
    {
        map gfx/water/surface2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll -0.02 -0.03
        tcMod stretch sin 1 0.006 0 0.7
    }
    {
        map textures/amace_vault/sg_eventhorizon_mask
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        tcMod rotate -4
        tcMod stretch sin 1.1 0.03 0 0.1
    }
}

// Water. :P Probably should replace this with caulk_water, as shotclip doesn't seem to work together with water surfaceparms.

textures/amace_vault/sg_eventhorizon_nodraw
{
	qer_editorimage	textures/system/nodraw
	surfaceparm	nodamage
	surfaceparm	nomarks
	surfaceparm	nodraw
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	shotclip
	surfaceparm nodrop
	q3map_nolightmap
}

// Ka-woosh! modelled water pillar

textures/amace_vault/sg_waterpillar
{
	qer_editorimage	textures/aMace_Vault/sg_eh1
	qer_trans	0.6
	surfaceparm	nonopaque
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	cull	twosided
	deformvertexes	wave	32 sin 0 8 0 1.2
    {
        map envmap
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen const 0.6//lightingSpecular
        tcGen environment
    }
    {
        animMap 0.8 textures/aMace_Vault/sg_eh1_glow textures/aMace_Vault/sg_eh2_glow textures/aMace_Vault/sg_eh3_glow textures/aMace_Vault/sg_eh4_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen wave inversesawtooth 0 1 0 0.8
        tcMod stretch sin 1 0.004 0 1.1
    }
    {
        animMap 0.8 textures/aMace_Vault/sg_eh2_glow textures/aMace_Vault/sg_eh3_glow textures/aMace_Vault/sg_eh4_glow textures/aMace_Vault/sg_eh1_glow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen wave sawtooth 0 1 0 0.8
        tcMod stretch sin 1 0.006 0 1.3
    }
}

// Subspace Wormhole Model Shader

textures/amace_vault/wormhole
{
	qer_editorimage	textures/amace_vault/wormhole_direction
	qer_trans	0.5
	surfaceparm	slick
	surfaceparm	nodamage
	surfaceparm	noimpact
	surfaceparm	nomarks
	surfaceparm	nonopaque
	surfaceparm	playerclip
	surfaceparm	monsterclip
	surfaceparm	trans
	q3map_nolightmap
	cull	twosided
    {
        map textures/amace_vault/wormhole
        blendFunc GL_ONE GL_ONE
        depthFunc disable
        glow
        rgbGen exactVertex
        alphaGen portal 250
        tcMod scale 54 1
        tcMod scroll -0.7 0
        tcMod turb 0 0.1 0 0.1
    }
    {
        map textures/amace_vault/wormhole2
        blendFunc GL_SRC_ALPHA GL_ONE
        depthFunc disable
        rgbGen exactVertex
        alphaGen portal 350
        tcMod scale 3 1
        tcMod scroll -0.06 -0.6
        tcMod turb 0 0.1 0 0.25
    }
    {
        map textures/amace_vault/wormhole3
        blendFunc GL_SRC_ALPHA GL_ONE
        depthFunc disable
        glow
        rgbGen exactVertex
        alphaGen portal 350
        tcMod scale 6 1
        tcMod scroll -1.7 -0.3
        tcMod turb 0 0.25 0 0.25
    }
}

// Caustics used in the Event Horizon - not anymore.

textures/amace_vault/sg_caustics1
{
	q3map_nolightmap
	cull	twosided
    {
        map textures/amace_vault/sg_caustics1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

textures/amace_vault/sg_caustics2
{
	q3map_nolightmap
	cull	twosided
    {
        map textures/amace_vault/sg_caustics2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

textures/amace_vault/sg_caustics3
{
	q3map_nolightmap
	cull	twosided
    {
        map textures/amace_vault/sg_caustics3
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

// Closing wormhole in event horizon, drawn on EFX cylinder

textures/amace_vault/sg_dissolve
{
	q3map_nolightmap
	cull	twosided
    {
        map textures/amace_vault/sg_dissolve
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
        tcMod scale 3 1
        tcMod scroll -0.7 0
    }
}

textures/amace_vault/sg_dissolvefiller
{
	qer_editorimage	models/effects/sphereofdoom_filler
	q3map_nolightmap
	cull	twosided
    {
        map models/effects/sphereofdoom_filler
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

// Water Shaders for maps, WiP, several versions/states planned. Tried animating, not good. Currently dual-overlayered photosourced water that mimics waves on top of the environment map. 

textures/ajawa_rock/muddywater_light
{
	qer_editorimage	gfx/water/surface
	q3map_shadeangle	200
	q3map_tesssize	128
	qer_trans	0.4
	surfaceparm	nomarks
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	trans
	q3map_material	Water
	deformvertexes	wave	128 sin 0 0.3 0.3 0.5
    {
        map textures/h_evil/waterf1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen const ( 0.321569 0.203922 0.121569 )
        tcMod scroll 0.01 -0.14
        tcMod turb 1 0.01 0 0.2
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        rgbGen exactVertex
        alphaGen lightingSpecular
        tcGen environment
    }
    {
        map envmap
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen identity
        tcGen environment
    }
    {
        map gfx/water/surface
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll 0.02 -0.05
        tcMod turb 1 0.01 0 0.3
    }
    {
        map gfx/water/surface2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll -0.02 -0.03
        tcMod stretch sin 1 0.006 0 0.7
    }
    {
        map $lightmap	// Hacky disable of the lightmap... If kept, it will make water pitch black in dark areas. :(
        alphaFunc GE128
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen lightingSpecular
    }
}

textures/ajawa_rock/muddywater_dark
{
	qer_editorimage	gfx/water/surface
	q3map_shadeangle	200
	q3map_tesssize	128
	qer_trans	0.4
	surfaceparm	nomarks
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	trans
	q3map_material	Water
	deformvertexes	wave	128 sin 0 0.3 0.3 0.5
    {
        map textures/h_evil/waterf1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen const ( 0.321569 0.203922 0.121569 )
        tcMod scroll 0.01 -0.14
        tcMod turb 1 0.01 0 0.2
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        rgbGen exactVertex
        alphaGen lightingSpecular
        tcGen environment
    }
    {
        map envmap
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcGen environment
    }
    {
        map gfx/water/surface
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll 0.02 -0.05
        tcMod turb 1 0.01 0 0.3
    }
    {
        map gfx/water/surface2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll -0.02 -0.03
        tcMod stretch sin 1 0.006 0 0.7
    }
    {
        map $lightmap	// Hacky disable of the lightmap... If kept, it will make water pitch black in dark areas. :(
        alphaFunc GE128
        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen lightingSpecular
    }
}

// Le WIP shaders - FINAL

textures/water/surface_waves
{
	qer_editorimage	gfx/water/surface_edim
	q3map_tesssize	128
	qer_trans	0.6
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
//	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
//	fogparms	( 0.0627451 0.129412 0.145098 ) 500.0
	cull	twosided
	deformvertexes	wave	128 sin 0 2.4 0 0.5
    {
        map envmap
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen const 0.4		// Modify this if need be. 
        tcGen environment
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        rgbGen exactVertex
        alphaGen lightingSpecular
        tcGen environment
    }
    {
        map gfx/water/surface
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll 0.02 -0.05
        tcMod turb 1 0.01 0 0.3
    }
    {
        map gfx/water/surface2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll -0.02 -0.03
        tcMod stretch sin 1 0.006 0 0.7
    }
}

// Alpha fade doesn't work (yet, Didz is looking into it), so, these shaders fade to rough values. 

textures/water/surface_waves_75
{
	qer_editorimage	gfx/water/surface_edim
	q3map_tesssize	128
	qer_trans	0.4
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
//	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
//	fogparms	( 0.0627451 0.129412 0.145098 ) 500.0
	cull	twosided
	deformvertexes	wave	128 sin 0 2.4 0 0.5
    {
        map envmap
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen const 0.4
        tcGen environment
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        rgbGen const ( 0.75 0.75 0.75 )
        alphaGen lightingSpecular
        tcGen environment
    }
    {
        map gfx/water/surface_75
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll 0.02 -0.05
        tcMod turb 1 0.01 0 0.3
    }
    {
        map gfx/water/surface2_75
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll -0.02 -0.03
        tcMod stretch sin 1 0.006 0 0.7
    }
}

textures/water/surface_waves_50
{
	qer_editorimage	gfx/water/surface_edim
	q3map_tesssize	128
	qer_trans	0.2
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
//	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
//	fogparms	( 0.0627451 0.129412 0.145098 ) 500.0
	cull	twosided
	deformvertexes	wave	128 sin 0 2.4 0 0.5
    {
        map envmap
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen const 0.2
        tcGen environment
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        rgbGen const ( 0.50 0.50 0.50 )
        alphaGen lightingSpecular
        tcGen environment
    }
    {
        map gfx/water/surface_50
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll 0.02 -0.05
        tcMod turb 1 0.01 0 0.3
    }
    {
        map gfx/water/surface2_50
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll -0.02 -0.03
        tcMod stretch sin 1 0.006 0 0.7
    }
}

textures/water/surface_waves_25
{
	qer_editorimage	gfx/water/surface_edim
	q3map_tesssize	128
	qer_trans	0.1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
//	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
//	fogparms	( 0.0627451 0.129412 0.145098 ) 500.0
	cull	twosided
	deformvertexes	wave	128 sin 0 2.4 0 0.5
    {
        map envmap
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen const 0.0
        tcGen environment
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        rgbGen const ( 0.25 0.25 0.25 )
        alphaGen lightingSpecular
        tcGen environment
    }
    {
        map gfx/water/surface_25
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll 0.02 -0.05
        tcMod turb 1 0.01 0 0.3
    }
    {
        map gfx/water/surface2_25
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll -0.02 -0.03
        tcMod stretch sin 1 0.006 0 0.7
    }
}

// Nice foamy surf

textures/water/surf
{
	qer_editorimage	gfx/water/surf
	qer_trans	0.6
	surfaceparm	nomarks
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	cull	twosided
    {
        map gfx/water/surf
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen wave sin 0 1 0.5 0.2
        tcMod turb 1 0.3 0 0.2
        tcMod scroll 0.14 0
        tcMod stretch sin 1 0.04 0 0.7
    }
    {
        map gfx/water/surf2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen wave sin 0 1 0 0.2
        tcMod turb 1 0.37 0.5 0.2
        tcMod scroll -0.23 0
        tcMod stretch sin 1 0.06 0.5 0.7
    }
}

textures/water/surface_cheap
{
	qer_editorimage	gfx/water/surface
	q3map_tesssize	128
	qer_trans	0.6
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	cull	back
	deformvertexes	wave	128 sin 0 0.4 0 0.5
    {
        map envmap
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen lightingDiffuse
        tcGen environment
    }
    {
        map envmap_spec
        blendFunc GL_SRC_ALPHA GL_ONE
        glow
        rgbGen lightingDiffuse
        alphaGen lightingSpecular
        tcGen environment
    }
}


// Last modified by: Silverfang on 2019-04-04, Previously Modified by BlasTech on 2010-08-18
// Returned Stargate stuff into the shader as there are some other dependencies on them

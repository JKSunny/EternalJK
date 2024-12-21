/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#ifndef VK_IMGUI_SHADER_NODES_GENERAL_H
#define VK_IMGUI_SHADER_NODES_GENERAL_H

class Q3N_Shader : public BaseNodeExt
{
public:
    Q3N_Shader()
    {
		uint32_t i;

        setTitle("Shader");
        setStyle(NodeStyle::ultra_violet());
		setDestroyable( false );

		// values
		addValue<bool>( 0, "noPicMip",			qfalse );
		addValue<bool>( 1, "noMipMaps",			qfalse );
		addValue<bool>( 2, "noTC",				qfalse );
		addValue<bool>( 3, "noglfog",			qfalse );
		addValue<bool>( 4, "portal",			qfalse );
		addValue<bool>( 5, "polygonOffset",		qfalse );
		addValue<bool>( 6, "entityMergable",	qfalse );
		addValue<float>( 7, "clampTime",		0.0f );
		addValue<int>( 8,	"surfacelight",		0 );

		// pins
        addIN<std::string>( "cull",		"", ConnectionFilter::isClass<Q3N_cull>() );
        addIN<std::string>( "sort",		"", ConnectionFilter::isClass<Q3N_sort>() );
        addIN<std::string>( "material", "", ConnectionFilter::isClass<Q3N_material>() );
        addIN<std::string>( "sun",		"", ConnectionFilter::isClass<Q3N_sun>() );
        addIN<std::string>( "sky",		"", ConnectionFilter::isClass<Q3N_skyParms>() );
        addIN<std::string>( "fog",		"", ConnectionFilter::isClass<Q3N_fogParms>() );
        addIN<std::string>( "deform",	"", ConnectionFilter::isClass<Q3N_deform>() );

		// grouped-pin
		for ( i = 0 ; i < MAX_SURFACE_PARMS; i++ )
			addIN<std::string>( (const char*)va("surfaceparm %d", i),	"", ConnectionFilter::isClass<Q3N_surfaceParam>() );

		for ( i = 0 ; i < MAX_SHADER_STAGES; i++ )
			addIN<std::string>( (const char*)va("stage %d", i),			"", ConnectionFilter::isClass<Q3N_Stage>() );

		setGroupedPin( "surfaceparm", MAX_SURFACE_PARMS, MAX_SURFACE_PARMS, true );
		setGroupedPin( "stage", MAX_SHADER_STAGES, MAX_SHADER_STAGES, true );
    }

	void draw() override
	{
		handleGroupedPins();
		drawValues();

		if ( isLinkChanged() )
		{
			num_surface_parms = 0;
			for ( uint32_t i = 0; i < MAX_SURFACE_PARMS; i++ )
			{
				if ( inPin( (const char *)va( "surfaceparm %d", i ) )->isConnected() )
					num_surface_parms++;
			}
		}
	}

	// hack to get hardcoded size of 31 for surfaceparms..
	const int MAX_SURFACE_PARMS = ARRAY_LEN( tess.shader->surfaceParams );
	
	int num_surface_parms = 0;
};

class Q3N_cull : public BaseNodeExt
{
public:
	Q3N_cull()
	{
		setTitle( "Cull" );
		setStyle( NodeStyle::cool_gray() );
		setTypes( n_cull_type_string, ARRAY_LEN(n_cull_type_string) );

		// pins
		addOUT<std::string>("cull", PinStyle::cyan())->behaviour([this]() {
			return "cull " + getTypeStr();
		});
	}
};

class Q3N_sort : public BaseNodeExt
{
public:
	Q3N_sort()
	{
		setTitle( "Sort" );
		setStyle( NodeStyle::cool_gray() );
		setTypes(n_sort_mode_string, ARRAY_LEN(n_sort_mode_string) );

		// values
		addValue<nodeValueCombo>(	0, "keyword", { 0, n_sort_type_string, ARRAY_LEN(n_sort_type_string) } );
		addValue<int>(				1, "index", 0 );

		// pins
		addOUT<std::string>("sort", PinStyle::cyan())->behaviour([this]() {
			std::string params;

			if ( getType() == N_SORT_MODE_KEYWORD )
			{
				auto option = getValue<nodeValueCombo>("keyword");
				params.append( option.selectables[option.active_idx] );
			}
			else if ( getType() == N_SORT_MODE_INDEX )
			{
				int index = getValue<int>( "index" );
				params.append( va( "%d", index ) );
			}

			return "sort " + params;
		});
	}

	void typeChanged( void )
	{
		if ( !isTypeChanged() )
			return;

		setValueDraw( "keyword", (getType() == N_SORT_MODE_KEYWORD) ? true : false );
		setValueDraw( "index", (getType() == N_SORT_MODE_INDEX) ? true : false );
	}

	void draw() override
	{
		drawTypeSelector();
		drawValues();
		typeChanged();
	}
};

class Q3N_surfaceParam : public BaseNodeExt
{
public:
	Q3N_surfaceParam()
	{
		setTitle( "Surface Param" );
		setStyle( NodeStyle::cool_gray() );
		setTypes( n_surface_params_type_string, ARRAY_LEN(n_surface_params_type_string) );

		// pins
		addOUT<std::string>("param", PinStyle::cyan())->behaviour([this]() {
			return "surfaceparm " + getTypeStr();
		});
	}
};

class Q3N_material : public BaseNodeExt
{
public:
	Q3N_material()
	{
		setTitle( "Material" );
		setStyle( NodeStyle::cool_gray() );
		setTypes( n_material_type_string, ARRAY_LEN(n_material_type_string) );

		// pins
		addOUT<std::string>("material", PinStyle::cyan())->behaviour([this]() {
			return "material " + getTypeStr();
		});
	}
};

class Q3N_sun : public BaseNodeExt
{
public:
	Q3N_sun()
	{
		setTitle( "Sun" );
		setStyle( NodeStyle::cool_gray() );

		vec3_t empty_3;
		Com_Memset( &empty_3, 0, sizeof(vec3_t) );

		// values
		addValue<vec3_t>(	0, "color",		empty_3 );
		addValue<vec3_t>(	1, "direction", empty_3 );

		// pins
		addOUT<std::string>("sun", PinStyle::cyan())->behaviour([this]() {
			auto color = getValue<vec3_t>( "color" );
			auto direction = getValue<vec3_t>( "direction" );

			std::string params = va( "%f %f %f %f %f %f", 
				color[0], color[1], color[2],
				direction[0], direction[1], direction[2]
			);

			return "sun " + params;
		});
	}
};

class Q3N_fogParms : public BaseNodeExt
{
public:
	Q3N_fogParms()
	{
		setTitle( "Fog" );
		setStyle( NodeStyle::cool_gray() );

		vec3_t empty_3;
		Com_Memset( &empty_3, 0, sizeof(vec3_t) );

		// values
		addValue<vec3_t>( 0,	"color",			empty_3 );
		addValue<float>( 1,		"depthForOpaque",	0 );

		// pins
		addOUT<std::string>("fog", PinStyle::cyan())->behaviour([this]() {
			auto color = getValue<vec3_t>( "color" );
			auto depthForOpaque = getValue<float>( "depthForOpaque" );

			std::string params = va( "( %f %f %f ) %f", color[0], color[1], color[2], depthForOpaque );

			return "fogParms " + params;
		});
	}
};

class Q3N_deform : public BaseNodeExt
{
public:
	Q3N_deform()
	{
		setTitle( "Deform");
		setStyle( NodeStyle::cool_gray() );
		setTypes( n_deform_type_string, ARRAY_LEN(n_deform_type_string) );

		vec3_t empty_3;
		Com_Memset( &empty_3, 0, sizeof(vec3_t) );

		// values
		addValue<float>(	0, "spread",		0.0f );		// wave

		addValue<vec3_t>(	1, "displace",		empty_3 );	// move

		addValue<float>(	2, "amplitude",		0.0f );		// normal
		addValue<float>(	3, "frequency",		0.0f );		// normal

		addValue<float>(	4, "bulgeWidth",	0.0f );		// bulge
		addValue<float>(	5, "bulgeHeight",	0.0f );		// bulge
		addValue<float>(	6, "bulgeSpeed",	0.0f );		// bulge

		// pins
		addIN<std::string>( "modifier", "", ConnectionFilter::isClass<Q3N_waveForm>() )->setRequired( true );

		addOUT<std::string>( "deform", PinStyle::cyan())->behaviour( [this]() {
			std::string params;
			params += getTypeStr() + " ";

			int type = getType();

			if ( type == N_DEFORM_WAVE )
			{
				params.append( va( "%f ", getValue<float>( "spread" ) ) );
			}
			else if ( type == N_DEFORM_MOVE )
			{
				auto displace = getValue<vec3_t>( "displace" );
				params.append( va( "%f %f %f ", displace[0], displace[1], displace[2] ) );
			}
			else if ( type == N_DEFORM_NORMALS )
			{
				auto amplitude = getValue<float>( "amplitude" );
				auto frequency = getValue<float>( "frequency" );
				params.append( va( "%f %f", amplitude, frequency ) );
			}
			else if ( type == N_DEFORM_BULGE )
			{
				auto bulgeWidth = getValue<float>( "bulgeWidth" );
				auto bulgeHeight = getValue<float>( "bulgeHeight" );
				auto bulgeSpeed = getValue<float>( "bulgeSpeed" );
				params.append( va( "%f %f %f", bulgeWidth, bulgeHeight, bulgeSpeed ) );
			}

			auto modifier = getInVal<std::string>( "modifier" );
			if ( !modifier.empty() )
				params.append( " " + modifier );

			return "deform " + params;
		});
	}

	void typeChanged( void )
	{
		if ( !isTypeChanged() )
			return;

		// values
		setValueDrawAll( false );

		switch ( getType() )
		{
			case N_DEFORM_WAVE:
				setValueDraw( "spread",		true );
				break;
			case N_DEFORM_MOVE:
				setValueDraw( "displace",		true );
				break;
			case N_DEFORM_NORMALS:
				setValueDraw( "amplitude",	true );
				setValueDraw( "frequency",	true );
				break;
			case N_DEFORM_BULGE:
				setValueDraw( "bulgeWidth",	true );
				setValueDraw( "bulgeHeight",true );
				setValueDraw( "bulgeSpeed",	true );
				break;
		}

		// pins
		inPin("modifier")->setDisabled( (getType() == N_DEFORM_WAVE || (getType() == N_DEFORM_MOVE) ) ? false : true );
	}

	void draw() override
	{
		drawTypeSelector();
		typeChanged();
		drawValues();
	}
};

class Q3N_skyParms : public BaseNodeExt
{
public:
	Q3N_skyParms()
	{
		setTitle( "Sky" );
		setStyle( NodeStyle::cool_gray() );

		// values
		addValue<int>( 0,		"cloudHeight",	512 );

		// pins
		addIN<std::string>( "outerbox", "-", ConnectionFilter::isClass<Q3N_Texture>() );
		//addIN<std::string>( "innerbox", "", ConnectionFilter::isClass<Q3N_Texture>() );	// unused

		addOUT<std::string>("sky", PinStyle::cyan())->behaviour([this]() {
			auto cloudHeight = getValue<int>( "cloudHeight" );
			auto outerbox = getInVal<std::string>( "outerbox" );

			std::string texture =	( (!outerbox.empty() ? outerbox : "-") );
			std::string cloud =		( (cloudHeight ? std::to_string(cloudHeight) : "-" ) );

			return "skyParms " + texture + " " + cloud + " -";
		});
	}
};

//
// global
//
class Q3N_Texture : public BaseNodeExt
{
public:
    Q3N_Texture()
    {
        setTitle("Image Texture");
        setStyle(NodeStyle::orange());

		addValue<char[MAX_QPATH * 2]>( 0, "##texture", "" );

		// pins
        addOUT<std::string>("texture", PinStyle::cyan())->behaviour([this]()
		{ 
			std::string texture( getValue<char[MAX_QPATH * 2]>( "##texture" ) );
			return texture;
		});

		setString<char[MAX_QPATH * 2]>( "##texture", "" );	// hack actually..
    }

    void draw() override
    {
		drawValues();
    }
};

//
// modifiers
//
class Q3N_waveForm : public BaseNodeExt
{
public:
    Q3N_waveForm()
    {
        setTitle("Modifier");
        setStyle(NodeStyle::cyan());
		setTypes( n_modifier_string, ARRAY_LEN(n_modifier_string) );

		// values
		addValue<float>(	0, "base",		0.0f );
		addValue<float>(	1, "amplitude",	0.0f );
		addValue<float>(	2, "phase",		0.0f );
		addValue<float>(	3, "frequency",	0.0f );
		
		// pins
        addOUT<std::string>( "modifier", PinStyle::cyan() )->behaviour([this]() { 

			std::string params = "";

			if ( !isTypeless() )
				params += getTypeStr();

			auto base		= getValue<float>( "base" );
			auto amplitude	= getValue<float>( "amplitude" );
			auto phase		= getValue<float>( "phase" );
			auto frequency	= getValue<float>( "frequency" );
			params.append( va( " %f %f %f %f", base, amplitude, phase, frequency ) );

			return params;
		});
    }

	bool isTypeless( void )
	{
		for ( auto &n : outPin("modifier")->getConnectedPinNodes() )
		{
			if ( static_cast<BaseNodeExt*>( n )->isTypeLessModifierSubNode() )
				return true;
		}

		return false;
	}

    void draw() override
    {
		// TODO: allow modifier to just have a single link ..
		if ( !isTypeless() )
			drawTypeSelector();

		drawValues();
	}
};
#undef TYPEDO
#endif // VK_IMGUI_SHADER_NODES_GENERAL_H
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

#ifndef VK_IMGUI_SHADER_NODES_STAGE_H
#define VK_IMGUI_SHADER_NODES_STAGE_H

class Q3N_Stage : public BaseNodeExt
{
public:
    Q3N_Stage()
    {
		uint32_t i;

        setTitle("Stage");
        setStyle(NodeStyle::green());

		// values
		addValue<bool>( 0, "glow",			qfalse );
		addValue<bool>( 1, "detail",		qfalse );
		addValue<bool>( 2, "depthWrite",	qfalse );

		// pins
        addIN<std::string>("map",			"", ConnectionFilter::isClass<Q3N_Map>());
        addIN<std::string>("normal map",	"", ConnectionFilter::isClass<Q3N_NormalMap>());
        addIN<std::string>("physical map",	"", ConnectionFilter::isClass<Q3N_PhyiscalMap>());
        addIN<std::string>("blend",			"", ConnectionFilter::isClass<Q3N_blendFunc>());
        addIN<std::string>("alpha",			"", ConnectionFilter::isClass<Q3N_alphaFunc>());
        addIN<std::string>("depth",			"", ConnectionFilter::isClass<Q3N_depthFunc>());
        addIN<std::string>("rgbGen",		"", ConnectionFilter::isClass<Q3N_rgbGen>());
        addIN<std::string>("alphaGen",		"", ConnectionFilter::isClass<Q3N_alphaGen>());
        addIN<std::string>("tcGen",			"", ConnectionFilter::isClass<Q3N_tcGen>());

        addOUT<std::string>("stage", PinStyle::cyan())->behaviour([this]()
		{ 
			return ""; 
		});

		// grouped-pin
		for ( i = 0 ; i < TR_MAX_TEXMODS; i++ )
			addIN<std::string>( (const char*)va("tcMod %d", i),	"", ConnectionFilter::isClass<Q3N_tcMod>());

		setGroupedPin( "tcMod", TR_MAX_TEXMODS, TR_MAX_TEXMODS, true );
    }

	void draw() override
	{
		handleGroupedPins();
		drawValues();
	}

	int num_texmods = 0;
};

//
// texture nodes
//
class Q3N_Map : public BaseNodeExt
{
public:
    Q3N_Map()
    {
        setTitle("Texture Map");
        setStyle(NodeStyle::orange());
		setTypes( n_map_types_string, ARRAY_LEN(n_map_types_string) );

		// values
		addValue<float>( 0, "speed", 0.0f );

		// pins
        addOUT<std::string>( "map", PinStyle::cyan() )->behaviour([this]() { 
			std::string params;
			params += getTypeStr();

			if ( getType() >= N_ANIMMAP )
			{
				auto speed = getValue<float>( "speed" );
				params.append( va( " %f", speed ) );
			}

			for ( uint32_t i = 0; i < MAX_IMAGE_ANIMATIONS; i++ )
			{
				auto texture = getInVal<std::string>( (const char *)va("texture %d", i) );
				if ( !texture.empty() )
					params += " " + texture;
			}

			return params; 
		});

		// grouped-pins (dynamic)
		for ( uint32_t i = 0; i < MAX_IMAGE_ANIMATIONS; i++ ) {
			auto pin = addIN<std::string>( (const char *)va("texture %d", i), "", ConnectionFilter::isClass<Q3N_Texture>() );
			if ( i <= 1 ) // mark first two required, animMap is pointless with just one texture anyway
				pin->setRequired( true );
		}
    }

	void typeChanged( void )
	{
		if ( !isTypeChanged() )
			return;
		
		// grouped-pins: type dependent
		setGroupedPin( 
			"texture", 
			(int)( (getType() < N_ANIMMAP) ? 1 : MAX_IMAGE_ANIMATIONS ),
			MAX_IMAGE_ANIMATIONS,							
			(bool)( (getType() >= N_ANIMMAP) ? true : false ) 
		);

		// values
		setValueDraw( "speed", (getType() >= N_ANIMMAP) ? true : false );
	}

    void draw() override
    {
        drawTypeSelector();
		typeChanged();
		handleGroupedPins();
		drawValues();
    }

	int num_textures = 0;
};

class Q3N_NormalMap : public BaseNodeExt
{
public:
    Q3N_NormalMap()
    {
        setTitle("Normal Map");
        setStyle(NodeStyle::normal_purple());
		setTypes( n_normal_map_types_string, ARRAY_LEN(n_normal_map_types_string) );

		// pins
		addIN<std::string>( "texture", "", ConnectionFilter::isClass<Q3N_Texture>() )->setRequired( true );

        addOUT<std::string>( "map", PinStyle::cyan() )->behaviour([this]() { 
			std::string params;
			params += getTypeStr();

			auto texture = getInVal<std::string>( "texture" );
			if ( !texture.empty() )
				params += " " + texture;

			return params; 
		});
    }
};

class Q3N_PhyiscalMap : public BaseNodeExt
{
public:
    Q3N_PhyiscalMap()
    {
        setTitle("Physical Map");
        setStyle(NodeStyle::cambridge_blue());
		setTypes( n_physical_map_types_string, ARRAY_LEN(n_physical_map_types_string) );

		// pins
		addIN<std::string>( "texture", "", ConnectionFilter::isClass<Q3N_Texture>() )->setRequired( true );

        addOUT<std::string>( "map", PinStyle::cyan() )->behaviour([this]() { 
			std::string params;
			params += getTypeStr();

			auto texture = getInVal<std::string>( "texture" );
			if ( !texture.empty() )
				params += " " + texture;

			return params; 
		});
    }
};

//
//	func nodes
//
class Q3N_alphaFunc : public BaseNodeExt
{
public:
    Q3N_alphaFunc()
    {
        setTitle("alphaFunc");
        setStyle(NodeStyle::cyan());
		setTypes( n_alpha_func_string, ARRAY_LEN(n_alpha_func_string) );

		// pins
        addOUT<std::string>( "alpha", PinStyle::cyan() )->behaviour([this]() { 
			return "alphaFunc " + getTypeStr();
		});
    }
};

class Q3N_depthFunc : public BaseNodeExt
{
public:
    Q3N_depthFunc()
    {
        setTitle("depthFunc");
        setStyle(NodeStyle::cyan());
		setTypes( n_depth_func_string, ARRAY_LEN(n_depth_func_string) );

		// pins
        addOUT<std::string>( "depth", PinStyle::cyan() )->behaviour([this]() { 
			return "depthFunc " + getTypeStr();
		});
    }
};

class Q3N_blendFunc : public BaseNodeExt
{
public:
	Q3N_blendFunc()
	{
		setTitle( "BlendFunc" );
		setStyle( NodeStyle::cyan() );
		
		// values
		addValue<nodeValueCombo>( 0,	"src",	{ 0, n_blend_func_src_type_string, ARRAY_LEN(n_blend_func_src_type_string) } );
		addValue<nodeValueCombo>( 1,	"dst",	{ 0, n_blend_func_dst_type_string, ARRAY_LEN(n_blend_func_dst_type_string) } );

		// pins
		addOUT<std::string>("blend", PinStyle::cyan())->behaviour([this]() {		
			auto src = getValue<nodeValueCombo>( "src" );
			auto dst = findValue( "dst" );

			std::string blend = src.selectables[src.active_idx];

			if ( dst->isDraw() )
			{
				auto dst_val = static_cast<NodeValue<nodeValueCombo>*>( dst )->val();
				blend.append( va( " %s", dst_val.selectables[dst_val.active_idx] ) );
			}

			return "blendFunc " + blend;
		});
	}

	void typeChanged( void )
	{
		if ( !isValueChanged() )
			return;

		// values
		auto src = getValue<nodeValueCombo>( "src" );

		// hide dst if src uses: add, filter or blend macro
		setValueDraw("dst", (src.active_idx < N_SRC_BLENDFUNC_ONE) ? false : true);
	}

	void draw() override
	{
		drawValues();
		typeChanged();
	}
};

//
// gen & mod nodes
//
class Q3N_rgbGen : public BaseNodeExt
{
public:
    Q3N_rgbGen()
    {
        setTitle("rgbGen");
        setStyle(NodeStyle::cyan());
		setTypes( n_rgbgen_type_string, ARRAY_LEN(n_rgbgen_type_string) );

		vec3_t empty_3;
		Com_Memset( &empty_3, 0, sizeof(vec3_t) );

		// values
		addValue<vec3_t>(0, "color", empty_3  );

		// pins
		addIN<std::string>( "modifier", "", ConnectionFilter::isClass<Q3N_waveForm>() )->setRequired( true );

        addOUT<std::string>( "rgbGen", PinStyle::cyan() )->behaviour([this]() { 
			std::string params;
			params += getTypeStr() + " ";

			if ( getType() == N_CGEN_WAVEFORM )
			{
				auto modifier = getInVal<std::string>( "modifier" );
				if ( !modifier.empty() )
					params.append( " " + modifier );
			}
			else if ( getType() == N_CGEN_CONST )
			{
				auto color = getValue<vec3_t>( "color" );
				params.append( va( "( %f %f %f )", color[0], color[1], color[2] ) );
			}

			return "rgbGen " + params; 
		});
    }

	void typeChanged( void )
	{
		if ( !isTypeChanged() )
			return;

		// values
		setValueDraw("color", (getType() == N_CGEN_CONST) ? true : false);		

		// pins
		inPin( "modifier" )->setDisabled( (getType() != N_CGEN_WAVEFORM) ? true : false );
	}

    void draw() override
    {
		drawTypeSelector();
		typeChanged();
		drawValues();
    }
};

class Q3N_alphaGen : public BaseNodeExt
{
public:
	Q3N_alphaGen()
	{
		setTitle( "alphaGen");
		setStyle( NodeStyle::cyan() );
		setTypes( n_alphagen_type_string, ARRAY_LEN(n_alphagen_type_string) );

		// values
		addValue<float>(	0, "range", 0.0f );
		addValue<float>(	1, "alpha", 0.0f );

		// pins
		addIN<std::string>( "modifier", "", ConnectionFilter::isClass<Q3N_waveForm>() )->setRequired( true );

		addOUT<std::string>( "alphaGen", PinStyle::cyan())->behaviour( [this]() {
			std::string params;
			params += getTypeStr();

			if ( getType() == N_AGEN_WAVEFORM )
			{
				auto modifier = getInVal<std::string>( "modifier" );
				if ( !modifier.empty() )
					params.append( " " + modifier );
			}
			else if ( getType() == N_AGEN_PORTAL )
			{
				auto range = getValue<float>( "range" );
				params.append( va( " %f", range ) );
			}
			else if ( getType() == N_AGEN_CONST )
			{
				auto alpha = getValue<float>( "alpha" );
				params.append( va( " %f", alpha ) );
			}

			return "alphaGen " + params; 
		});
	}

	void typeChanged( void )
	{
		if ( !isTypeChanged() )
			return;

		// values
		setValueDraw("range", (getType() == N_AGEN_PORTAL) ? true : false);
		setValueDraw("alpha", (getType() == N_AGEN_CONST) ? true : false);

		// pins
		inPin( "modifier" )->setDisabled( (getType() != N_AGEN_WAVEFORM) ? true : false );
	}

	void draw() override
	{
		drawTypeSelector();
		typeChanged();
		drawValues();
	}
};

class Q3N_tcGen : public BaseNodeExt
{
public:
	Q3N_tcGen()
	{
		setTitle( "tcGen" );
		setStyle( NodeStyle::cyan() );
		setTypes( n_tcgen_type_string, ARRAY_LEN(n_tcgen_type_string) );

		vec3_t empty_3;
		Com_Memset( &empty_3, 0, sizeof(vec3_t) );

		// values
		addValue<vec3_t>(	0, "axis_s", empty_3 );
		addValue<vec3_t>(	1, "axis_t", empty_3 );

		// pins
		addOUT<std::string>("tcGen", PinStyle::cyan())->behaviour([this]() {
			std::string params;
			params += getTypeStr();

			if ( getType() == N_TCGEN_VECTOR )
			{
				auto vector1 = getValue<vec3_t>( "axis_s" );
				auto vector2 = getValue<vec3_t>( "axis_t" );
				params.append( va( " ( %f %f %f ) ( %f %f %f )", 
					vector1[0], vector1[1], vector1[2],
					vector2[0], vector2[1], vector2[2]
				) );
			}

			return "tcGen " + params; 
		});
	}

	void typeChanged(void)
	{
		if (!isTypeChanged())
			return;

		// values
		setValueDraw("axis_s", (getType() == N_TCGEN_VECTOR) ? true : false);
		setValueDraw("axis_t", (getType() == N_TCGEN_VECTOR) ? true : false);
	}

	void draw() override
	{
		drawTypeSelector();
		typeChanged();
		drawValues();
	}
};

class Q3N_tcMod : public BaseNodeExt
{
public:
	Q3N_tcMod()
	{
		setTitle( "tcMod" );
		setStyle( NodeStyle::cyan() );
		setTypes( n_tcmod_type_string, ARRAY_LEN(n_tcmod_type_string) );

		vec2_t empty_2;
		Com_Memset( &empty_2, 0, sizeof(vec2_t) );

		// values
		addValue<vec2_t>(	0, "matrix_x",	empty_2 );
		addValue<vec2_t>(	1, "matrix_y",	empty_2 );
		addValue<vec2_t>(	2, "translate", empty_2 );
		addValue<float>(	3, "rotate", 0.0f );

		// pins
		addIN<std::string>("modifier", "", ConnectionFilter::isClass<Q3N_waveForm>())->setRequired( true );

		addOUT<std::string>( "tcMod", PinStyle::cyan() )->behaviour([this]() {
			std::string params;
			params += getTypeStr();

			if ( getType() == N_TMOD_ROTATE )
			{
				auto rotate = getValue<float>( "rotate" );
				params.append( va( " %f", rotate ) );
			}
			else if ( getType() == N_TMOD_SCROLL || getType() == N_TMOD_SCALE )
			{
				auto translate = getValue<vec2_t>( "translate" );
				params.append( va( " %f %f", translate[0], translate[1] ) );
			}
			else if ( getType() == N_TMOD_TRANSFORM )
			{
				auto translate = getValue<vec2_t>( "translate" );
				auto matrix_x = getValue<vec2_t>( "matrix_x" );
				auto matrix_y = getValue<vec2_t>( "matrix_y" );
				params.append( va( " %f %f %f %f %f %f", 
					matrix_x[0], matrix_x[1],
					matrix_y[0], matrix_y[1],
					translate[0], translate[1] 
				) );
			}
			else if ( getType() == N_TMOD_STRETCH || getType() == N_TMOD_TURBULENT )
			{
				auto modifier = getInVal<std::string>( "modifier" );
				if ( !modifier.empty() )
					params.append( " " + modifier );
			}

			return "tcMod " + params; 
		});
	}
	
	// override to handle [tcmod 'turb'] edge case
	// which has a typeless modifier sub-node
	void setType( int type ) override 
	{
		BaseNodeExt::setType( type );

		setTypeLessModifierSubNode( (type == N_TMOD_TURBULENT) ? true : false );
	}

	void typeChanged(void)
	{
		if ( !isTypeChanged() )
			return;

		// values
		setValueDrawAll( false );

		switch ( getType() )
		{
			case N_TMOD_ENTITY_TRANSLATE:
			case N_TMOD_TURBULENT:
			case N_TMOD_STRETCH: 
				break;
			case N_TMOD_ROTATE:
				setValueDraw( "rotate", true );
				break;
			case N_TMOD_SCROLL:
			case N_TMOD_SCALE:
				setValueDraw( "translate", true );
				break;
			case N_TMOD_TRANSFORM:
				setValueDrawAll( true );
				setValueDraw( "rotate", false );
				break;
		}

		// pins
		inPin("modifier")->setDisabled( (getType() == N_TMOD_STRETCH || (getType() == N_TMOD_TURBULENT) ) ? false : true);
	}

	void draw() override
	{
		drawTypeSelector();
		typeChanged();
		drawValues();
	}
};

#undef TYPEDO
#endif // VK_IMGUI_SHADER_NODES_H
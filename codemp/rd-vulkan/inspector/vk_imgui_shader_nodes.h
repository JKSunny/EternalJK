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

#ifndef VK_IMGUI_SHADER_NODES_H
#define VK_IMGUI_SHADER_NODES_H

using namespace ImFlow;
extern ImNodeFlow node_editor;

#define N_NODE_TYPES \
	/* general */ \
    TYPE_DO( Q3N_cull,			cull			) \
    TYPE_DO( Q3N_sort,			sort			) \
    TYPE_DO( Q3N_surfaceParam,	surfaceParm		) \
    TYPE_DO( Q3N_material,		material		) \
    TYPE_DO( Q3N_sun,			sun				) \
    TYPE_DO( Q3N_fogParms,		fogParms		) \
    TYPE_DO( Q3N_deform,		deform			) \
    TYPE_DO( Q3N_skyParms,		skyParms		) \
	\
	/* stage */ \
    TYPE_DO( Q3N_Stage,			stage			) \
    TYPE_DO( Q3N_Map,			texture map		) \
    TYPE_DO( Q3N_NormalMap,		normal map		) \
    TYPE_DO( Q3N_PhyiscalMap,	physical map	) \
    TYPE_DO( Q3N_alphaFunc,		alphaFunc		) \
    TYPE_DO( Q3N_depthFunc,		depthFunc		) \
    TYPE_DO( Q3N_blendFunc,		blendFunc		) \
    TYPE_DO( Q3N_rgbGen,		rgenGen			) \
    TYPE_DO( Q3N_alphaGen,		alphaGen		) \
    TYPE_DO( Q3N_tcGen,			tcGen			) \
    TYPE_DO( Q3N_tcMod,			tcMod			) \
	\
	/* global */ \
	TYPE_DO( Q3N_Texture,		image			) \
	\
	/* modifiers */ \
	TYPE_DO( Q3N_waveForm,		modifier		) \

//
// node factory
// instantiate nodes using class identifier as string
//
class NodeFactory 
{
public:
	struct NodeTypes {
		std::string alias;
		std::function<std::shared_ptr<BaseNode>( const ImVec2 &pos )> addNode;
		std::function<std::shared_ptr<BaseNode>( const ImVec2 &pos )> placeNodeAt;
	};

	/**
	 * @brief Registers a node type with an alias.
	 * Stores a lambda function that creates a new node in the node editor.
	 *
	 * @tparam <className> The class identifier
	 * @param type The class identifier as string.
	 * @param alias A user-friendly name for the node type.
	 */
    template<typename T>
    void registerNode( const std::string& type, const std::string& alias ) 
	{
		nodes[type].alias = alias;
        nodes[type].addNode = []( const ImVec2 &pos ) { return node_editor.addNode<T>( pos ); };
        nodes[type].placeNodeAt = []( const ImVec2 &pos ) { return node_editor.placeNodeAt<T>( pos ); };
    }

	/**
	 * @brief Return map of registered node types
	 */
	std::unordered_map<std::string, NodeTypes> *getNodes( void ) { return &nodes; }

	/**
	 * @brief Create new node instance of 'type'
	 *
	 * @param type The class identifier as string.
	 */
    std::shared_ptr<BaseNode> addNode( const std::string& type, const ImVec2 &pos ) 
    {
      auto found = nodes.find( type );

        if ( found != nodes.end() )
            return found->second.addNode( pos );

        return nullptr;
    }

	/**
	 * @brief Create and place (using screen coords) a new node instance of 'type'
	 *
	 * @param type The class identifier as string.
	 */
    std::shared_ptr<BaseNode> placeNodeAt( const std::string& type, const ImVec2 &pos ) 
    {
      auto found = nodes.find( type );

        if ( found != nodes.end() )
            return found->second.placeNodeAt( pos );

        return nullptr;
    }
private:
	std::unordered_map<std::string, NodeTypes> nodes;
};

//
// global
//
 
/* @brief combines [words/sm, format and node-uid] per syntax token in a matching sized lists
* @brief See g_shaderGeneralFormats and g_shaderStageFormats
* @brief Example:
* 
* @brief REGEX SM	: IN "surfaceparm water"	OUT	words	[0=surfaceparm, 1=water]		// from shaderText
* @brief FORMAT		: IN "surfaceparm %s"		OUT	tokens	[0=surfaceparm, 1=%s]
* @brief NODE-UID	: IN "key type"				OUT node_uid[0=key,			1=type]
*/
class RegexMatchInfo {
public:
	static constexpr size_t max = 25;		// 15 would even suffice I suppose
	std::array<std::string, max> words;
	std::array<std::string, max> tokens;
	std::array<std::string, max> node_uid;
	size_t size = 0;

    void shift( size_t count ) 
	{
		if ( !count )
			return;

        assert( count <= size );

		for ( size_t i = 0; i <= size - count; ++i ) 
		{
			size_t j = i + count;

			words[i]	= words[j];
			tokens[i]	= tokens[j];
			node_uid[i] = node_uid[j];

			words[j] = tokens[j] = node_uid[j] = "";
		}

		size -= count;
    }
};

struct nodeValueCombo {
	uint32_t	active_idx;
	const char	**selectables;
	uint32_t	size;
};

class BaseNodeExt;
class NodeValueBase
{
public:
	virtual void draw( void ) { }
	const char	*getUid()			{ return m_uid; }
	BaseNodeExt *getParent()		{ return m_parent; }

	bool isDraw( void )				{ return m_draw; }
	bool isDisabled( void )			{ return m_disabled; }
	bool isChanged( void )			{ return m_changed; }
	void setDraw( bool state )		{ m_draw = state; }
	void setDisabled( bool state )	{ m_disabled = state; }
	void setChanged( bool state )	{ 
		m_changed = state; 
		
		if ( state )
			node_editor.setFlag( SHADER_NODES_MODIFIED | SHADER_NODES_UPDATE_TEXT ); 
	}

	[[nodiscard]] virtual const std::type_info& getDataType() const = 0;

protected:
	const char	*m_uid;
	BaseNodeExt *m_parent;
	bool m_draw = true;
	bool m_disabled = false;
	bool m_changed = false;
};

template<class T> class NodeValue : public NodeValueBase
{
public:
	explicit NodeValue( const char *uid, T val, BaseNodeExt *parent ) 
	{
		m_uid = uid;
		m_parent = parent;

		Com_Memcpy( &value, &val, sizeof( T ) );
	}

	void set( T val )						{ Com_Memcpy( &value, &val, sizeof( value ) );	setChanged( true ); }
	void setVector( float *vector )			{ Com_Memcpy( &value, vector, sizeof( T ) );	setChanged( true ); }
	void setString( const char *str )		{ Q_strncpyz( value, str, sizeof( T ) );		setChanged( true ); }
	void setCombo( int val )				{ value.active_idx = val;						setChanged( true ); }
	const T& val( void )					{ return value; }

	inline void draw( const char *label, int &value )			{ if ( ImGui::DragInt( label, &value, 1) )								{ setChanged( true ); } }
	inline void draw( const char *label, float &value )			{ if ( ImGui::DragFloat( label, &value, 0.5f, 0.0f, 0.0f, "%.2f" ) )	{ setChanged( true ); } }
	inline void draw( const char *label, char *value )			{ if ( ImGui::InputText( label, value, sizeof(T) ) )					{ setChanged( true ); } }
	inline void draw( const char *label, vec4_t &value )		{ if ( ImGui::DragFloat4( label, value, 0.5f, 0.0f, 0.0f, "%.2f" ) )	{ setChanged( true ); } }
	inline void draw( const char *label, vec3_t &value )		{ if ( ImGui::DragFloat3( label, value, 0.5f, 0.0f, 0.0f, "%.2f" ) )	{ setChanged( true ); } }
	inline void draw( const char *label, vec2_t &value )		{ if ( ImGui::DragFloat2( label, value, 0.5f, 0.0f, 0.0f, "%.2f" ) )	{ setChanged( true ); } }
	inline void draw( const char *label, bool &value )			{ if ( ImGui::Checkbox( label, &value ) )								{ setChanged( true ); } }
	inline void draw( const char *label, nodeValueCombo &value )
	{ 
		const char *type = value.selectables[value.active_idx];

		ImGui::SetNextItemWidth( 200.f );
		if ( ImGui::BeginCombo( label, type, ImGuiComboFlags_HeightLarge ) )
		{
			for ( uint32_t i = 0; i < value.size; i++ )
			{
				bool is_selected = ( type == value.selectables[i]);

				if ( ImGui::Selectable( value.selectables[i], is_selected ) )
				{
					setCombo( i );
					setChanged( true );
				}

				if ( is_selected )
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	void draw( void ) override	
	{ 
		if ( !isDraw() )
			return;

		if ( isDisabled() || getParent()->isDisabled() ) ImGui::BeginDisabled();

		ImGui::SetNextItemWidth(100.f);

		draw( va( "%s##%s", getUid(), getUid() ), value ) ;

		if ( isDisabled() || getParent()->isDisabled() ) ImGui::EndDisabled();
	}	

	[[nodiscard]] const std::type_info& getDataType() const override { return typeid(T); };
private:
	T		value;
};

//
// extended base node
//
class BaseNodeExt : public ImFlow::BaseNode
{
public:
	void draw() override
	{
		drawTypeSelector();
		drawValues();
	}

	void postDraw() override
	{
		if ( isTypeChanged() )
			resetTypeChanged( false );

		if ( isValueChanged() )	// optimization: see method description
			for ( auto &val : values )
				val.second->setChanged( false );
	}

	/**
	* @brief Check if the node is disabled based on the state of its connected parent pins. 
	* Only set disabled if all of its parent pins are disabled.
	*/
	void updateNodeState( void ) override 
	{
		uint32_t link_num = 0;
		uint32_t disabled_num = 0;

		for ( auto &p: getOuts() ) {
			for ( auto &linked_pin : p->getConnectedPins() )
			{
				link_num++;
				if ( linked_pin->isDisabled() )
					disabled_num++;
			}
		}

		setDisabled( ( link_num < 1 ) ? false : ( link_num > disabled_num ? false : true ) );
    }

	struct groupedPin {
		const char	*key;
		int			draw;
		int			draw_max;
		int			total;
		bool		expand;
	};

	/**
	* @brief Mark pins with uid "key %d" as a group.
	* Expects valid InPins with uid "key %d", where %d starts from 0 and goes up to the total.
	* 
	* @param key          The base name of the pins without the %d suffix.
	* @param draw_max     The maximum number of pins to draw (can be dynamic based on node type).
	* @param total        The total number of InPins created.
	* @param expand       If true, will expand 1 empty pin at the end instead of drawing all empty pins. If available
	*/
	void setGroupedPin( const char *key, const int draw_max, const int total, bool expand )
	{
		// update
		for ( uint32_t i = 0; i < groupedPins_count; ++i )
		{
			if ( !strncmp( groupedPins[i].key, key, strlen( key ) ) )
			{
				auto &gpin = groupedPins[i];
				gpin.draw_max = draw_max;
				gpin.total = total;
				gpin.expand = expand;
				return;
			}
		}

		// insert
		groupedPins[groupedPins_count++] = { key, 0, draw_max, total, expand };
	}

	/**
	* Handle pin groups, restrict num pins drawn, state and auto-expansion
	*/
	virtual void handleGroupedPins( void ) 
	{
		if ( !isLinkChanged() && !isTypeChanged() )
			return;

		uint32_t i, j;

		for ( i = 0; i < groupedPins_count; ++i )
		{
			auto &gpin = groupedPins[i];

			for ( j = 0; j < groupedPins[i].total; j++ )
			{
				const char *uid = va( "%s %d", groupedPins[i].key, j );

				// find pins connected pins to draw.
				if ( inPin( uid )->isConnected() )
					gpin.draw = j + 1;	

				// always draw first pin then active pins then one empty if expansion is enabled
				inPin( uid )->setDraw( ( j == 0 || j < (gpin.draw + (int)gpin.expand) ) ? true : false );

				// set pin disabled if it exceeds the max for this type
				if ( isTypeChanged() )
					inPin( uid )->setDisabled( j >= gpin.draw_max ? true : false );
			}
		}
	}

	//
	// types
	//
	//std::array<const char*, 50> types; // public
	const char **types; // public
	size_t types_size = 0;

	inline std::string getTypeStr( void )			{ return std::string( types[type_idx]); }
	inline int		getType( void )					{ return type_idx; }
	inline bool		isTypeChanged( void )			{ return type_changed; }
	void			resetTypeChanged( bool state )	{ type_changed = false; }
	void			setTypes( const char **list, const size_t size )	
	{ 
		// copy, look into direct reference later ..
		//for ( uint32_t i = 0; i < size; i++ )
		//	types[i] = list[i];

		types = list;
		types_size = size;
	}

	virtual void setType( int type )	
	{ 
		type_idx = type; 
		type_changed = true; 
		node_editor.setFlag( SHADER_NODES_MODIFIED | SHADER_NODES_UPDATE_TEXT ); 
	}
	/*
	* @brief handle [tcmod 'turb'] edge case by marking the modifier sub-node typeless
	* condider making function name global global whenever there are more edge cases?
	* 
	* @param state New state
	*/
	void setTypeLessModifierSubNode( bool state )	override	{ typeless_modifier_sub_node = state; }

	/**
	* @return True if modifier sub-node is typeless
	*/
	bool isTypeLessModifierSubNode( void )			override	{ return typeless_modifier_sub_node; }

	/*
	* @brief Draw dropdown selection menu with available types.
	* Trigger setType handler when modified
	*/
	virtual void drawTypeSelector( void )
	{
		if ( !types_size )
			return;

        const char *type = types[type_idx];

		if ( isDisabled() ) ImGui::BeginDisabled();
		
		ImGui::SetNextItemWidth( 100.f );
		if ( ImGui::BeginCombo( "type##type", type, ImGuiComboFlags_HeightLarge ) )
		{
			for ( int i = 0; i < types_size; i++ )
			{
				bool is_selected = ( type == types[i] );

				if ( ImGui::Selectable( types[i], is_selected ) )
					setType( i );

				if ( is_selected )
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if ( isDisabled() ) ImGui::EndDisabled();
	}

	//
	// values
	//
	std::map<int, std::shared_ptr<NodeValueBase>> getValues() { return values; }
	void drawValues()	{ for ( auto &val : values ) val.second->draw(); }

	// rather have local 'bool value_changed' which state can be set from NodeValue class
	// this does the job for now ..
	bool isValueChanged( void )	{ 
		for ( auto &val : values ) { 
			if ( val.second->isChanged() ) 
				return true; 
		} 
		return false; 
	}

	void setValueDrawAll( bool state )								{ for ( auto &val : values )	val.second.get()->setDraw( state );  }
	void setValueDraw( const char *key, bool state )				{ auto val = findValue( key );	val->setDraw( state ); }
	void setValueDisabledAll( const char *key, bool state )			{ for ( auto &val : values )	val.second.get()->setDisabled( state );  }
	void setValueDisabled( const char *key, bool state )			{ auto val = findValue( key );	val->setDisabled( state ); }

	NodeValueBase *findValue( const char *key ) 
	{
		auto it = std::find_if( values.begin(), values.end(), [&key]( const std::pair<int, std::shared_ptr<NodeValueBase>> &p ) 
			{ return strcmp( p.second->getUid(), key ) == 0; } );
		assert( it != values.end() && "Value not found!" );
		return it->second.get();
	}

	/**
	 * Parses and processes a RegexMatchInfo object, handling vectors, floats, and integers.
	 *
	 * @param parms A reference to a RegexMatchInfo object containing tokens, words, and node_uids.
	 * @param dismissParms Flag to clear parsed parameters from RegexMatchInfo; default is true.
	 */
	void parseParamValues( RegexMatchInfo &parms, bool dismissParms = true )
	{
		size_t i, j;

		typedef struct {
			std::string name;		// name/uid is ':' prefix
			vec4_t		vector;		// restricted to vec4_t by ImGui. good enough 
			uint32_t	count;
		} vectorBuffer;

		std::array<vectorBuffer, 10> vectors;	// 5 would probably be enough ..
		size_t vectors_found= 0;

		bool keepLast = false;

		// handle/collect vectors
		const auto l_handle_vector = [&]( void ) 
			{
				std::vector<std::string> info;
				info.reserve( 2 );
				text_editor.split( parms.node_uid[i], ":", info );

				float value = std::stof( parms.words[i] );

				const auto &it = std::find_if( vectors.begin(), vectors.end(), [&info](const vectorBuffer& item )
					{ return item.name.compare( info[0] ) == 0; } );
				
				// initialize new vector
				if ( it == vectors.end() )	
				{
					vectors[vectors_found].name = info[0];
					vectors[vectors_found].vector[std::stoi( info[1] )] = value;
					vectors[vectors_found].count = 1;
					vectors_found++;
					return;
				}
				
				it->vector[std::stoi( info[1] )] = value;
				it->count++;
			};

		for ( i = 0; i < parms.size; i++ )
		{
			if ( Q_stricmp (parms.tokens[i].c_str(), "%i") == 0 )
			{
				if ( strstr( parms.node_uid[i].c_str(), ":" ) )
					l_handle_vector();
				else
					setValue<int>( parms.node_uid[i].c_str(), parms.words[i].c_str() );

				continue;

			}
			else if ( Q_stricmp (parms.tokens[i].c_str(), "%f") == 0 )
			{
				if ( strstr( parms.node_uid[i].c_str(), ":" ) )
					l_handle_vector();
				else 
					setValue<float>( parms.node_uid[i].c_str(), parms.words[i].c_str() );

				continue;
			}

			else if ( Q_stricmp (parms.tokens[i].c_str(), "%c") == 0 )
			{
				l_handle_vector();
				continue;
			}

			// ignore
			else if ( Q_stricmp (parms.tokens[i].c_str(), "(") == 0 )
				continue;
			else if ( Q_stricmp (parms.tokens[i].c_str(), ")") == 0 )
				continue;
			else if ( Q_stricmp (parms.node_uid[i].c_str(), "void") == 0 )
				continue;
		
			// sub node begins
			else if ( Q_stricmp (parms.node_uid[i].c_str(), "modifier") == 0 )
				break;

			break; // for textures? dont recall.. oops
		}

		// set found vectors
		for ( j = 0; j < vectors_found; ++j )
		{
			switch( vectors[j].count )
			{
				case 2:
					setVector<vec2_t>( vectors[j].name.c_str(), vectors[j].vector );
					break;
				case 3:
					setVector<vec3_t>( vectors[j].name.c_str(), vectors[j].vector );
					break;
				case 4:
					setVector<vec4_t>( vectors[j].name.c_str(), vectors[j].vector );
					break;
				default:
					Com_Printf( "vector count [%d] unsupported", vectors[j].count );
					break;
			}
		}

		if ( dismissParms && i > 0 )
			parms.shift( i );
	}

	template<typename T>
    void addValue( int order, const char *key, T value )
	{
		values.emplace( std::make_pair( order, std::make_shared<NodeValue<T>>( key, value, this ) ) );
	}

	// auto cast
	template<typename T>
	void setValue( const char *key, const char *value )
	{
		if constexpr ( std::is_integral<T>::value )
			static_cast<NodeValue<T>*>( findValue( key ) )->set( std::atoi( value ) );

		else if ( std::is_floating_point<T>::value ) 
			static_cast<NodeValue<T>*>( findValue( key ) )->set( std::atof( value ) );
	}

	// float & int
	template<typename T>
	void setValue( const char *key, float value )
	{
		static_cast<NodeValue<T>*>( findValue( key ) )->set( value );
	}

	template<typename T>
	void setVector( const char *key, float *value )
	{
		static_cast<NodeValue<T>*>( findValue( key ) )->setVector( value );
	}

	// char / string
	template<typename T>
	void setString( const char *key, const char *value )
	{
		static_cast<NodeValue<T>*>( findValue( key ) )->setString( value );
	}

	template<typename T>
	void setCombo( const char *key, int value )
	{
		static_cast<NodeValue<T>*>( findValue( key ) )->setCombo( value );
	}

	template<typename T>
	const T& getValue( const char *key )
	{
		return static_cast<NodeValue<T>*>( findValue( key ) )->val();
	}
protected:
	// handle edge case [tcmod 'turb'] having typeless modifier sub-node..
	bool	typeless_modifier_sub_node = false;

	// node type
	bool	type_changed = true;
	int		type_idx = 0;

	// values
	std::map<int, std::shared_ptr<NodeValueBase>> values;

	// grouped-pins stores pins with uid "key %d"
	std::array<groupedPin, 50> groupedPins;	// usage should be max 32 for textures
	size_t groupedPins_count = 0;
};

// forward declare node types
#define TYPE_DO( _className, _alias ) \
	class _className;
N_NODE_TYPES
#undef TYPE_DO

//
// parsers
//
class BaseParser {
public:
    virtual ~BaseParser() = default;
};

class TextToNodesParser : public BaseParser {
public:
    void parse_general( RegexMatchInfo &parms );
    void parse_stage( const int stage, RegexMatchInfo &parms );
    void parse( void );

private:
	const ImVec2	node_margin				= ImVec2( 400.0f, 25.0f);
	const ImVec2	shader_node_position	= ImVec2( 700, 100 );
	const ImVec2	stage_node_position		= shader_node_position - ImVec2( node_margin.x, -150 );
	const ImVec2	general_nodes_position	= shader_node_position - ImVec2( node_margin.x, 150 );
};

class NodesToTextParser : public BaseParser {
public:
    void parse_general( BaseNodeExt *base_shader );
    void parse_stage( BaseNodeExt *stage );
    void parse( void );
    void parse_singular_values( BaseNodeExt &node );

    std::string shaderText = "";
};
#undef TYPEDO
#endif // VK_IMGUI_SHADER_NODES_H
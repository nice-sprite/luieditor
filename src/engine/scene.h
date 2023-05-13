//
// Created by coxtr on 12/14/2021.
//

#ifndef T7LUAEDITOR_SCENE_H
#define T7LUAEDITOR_SCENE_H

#include "ray_cast.h"
#include "renderer.h"
#include <d3d11.h>
#include <imgui.h>
#include <unordered_map>
#include <vector>


// every UIQuad has
// anchors for top left right bottom
// left, right, top, bottom bounds/position
// x, y, z rotation
// scale&zoom
// rgba
// 5 shaderVectors (vec4) 0, 1, 2, 3, 4 // I do want these because
// writing/porting UI material shaders would be sick parent element, siblings,
// children priority animation time left, animationDuration, animation name UI
// model index
//
// I think text elems are going to be existence based
// if text_elems[current_elem_index] then its a text element
// otherwise its an image element
// ^^^^CHECK if this approach is even reasonable first.
//
// what about layout elements?
// for instance, a horizontal UI list should be usable.
// ui lists are in charge of layout of their children. When we go to render,
// could we have a seperate array of layout widgets like this: struct
// HorizontalListLayout {
//      int index;
//      int children[]; // list of indices for children of this box
// }

namespace ui 
{

    struct UIElementBoundingBox
    {
        float left;
        float right;
        float top;
        float bottom; 
    };

    enum class UIElementType
    {
        Element, // the "container" type
        Image,
        Text,
        TightText,
        HorizontalList,
        VerticalList,
        GridLayout,
        Custom,
    };

    // TODO: 
    // since UIElements allocate out of a contiguous block of memory, 
    // want to try using relative pointers to make de/serialization easier?
    struct UIElement
    {
        UIElementType type;
        UIElement* parent; // depth - 1
        UIElement* previous_sibling; // on the same level
        UIElement* next_sibling;
        UIElement* first_child; // depth + 1
        UIElement* last_child;

        std::string name;
        UIElementBoundingBox box;
        glm::vec4 color;
        f32 rotation_x;
        f32 rotation_y;
        f32 rotation_z;
        union {
            struct Font* font; // some kind of reference to a font, if its a text element
            Texture2D* image;
            struct UserRenderData* user_renderer;
        };
        i32 priority; 
    };

    struct ElementTreeState
    {
        UIElement* selected_element;  // do not dereference!
    };

    /* Holds data about the widget hierarchy and layout */
    struct SceneDef 
    {
        u32 bytes_per_element;
        u32 indices_per_element;
        u32 used_element_count; // number of active elements
        u32 max_element_count; 
        UIElement* elements;  // element pool 
        UIElement* root_element; // can just make the first element always be the root?

        ElementTreeState tree_state;

        // glm::vec4 bounding_boxs[MaxQuads]{}; // left, right, top, bottom
        // glm::vec4 rotations[MaxQuads]{};     // maybe make this a transform Matrix?
        // glm::vec4 colors[MaxQuads]{};        // maybe compress this into a u32?
        //std::string name[MaxQuads]{};
    };

    SceneDef scene_create(u32 element_count);
    UIElement* alloc_element();
    UIElement* make_element(SceneDef* scene, std::string name, f32 left, f32 right, f32 top, f32 bottom, i32 layer, glm::vec4 color);
    void scene_add_element(SceneDef* scene, UIElement element);
    void scene_create_root(SceneDef* scene, std::string root_name, f32 x, f32 y, f32 width, f32 height, glm::vec4 color);
    void scene_destroy(SceneDef* scene); // frees element pool memory
    glm::vec2 scene_calculate_root_center(SceneDef* scene);

    void editor_update(SceneDef* scene, struct Texture2D& texture);
};


static constexpr auto MaxQuads = 10000;
static constexpr auto MaxWidgetNameSize = 128;
static constexpr auto MaxIndices = 6 * MaxQuads; // there are 6 indices per quad

// add constant buffer for transforms?
struct GfxSceneResources 
{
    ID3D11Buffer* vertex_buffer;
    ID3D11Buffer* index_buffer;
    ID3D11InputLayout* vertex_layout;
    GfxShaderProgram scene_shader;
    RenderTarget render_target;
    //TODO make a "Material" def and live shader reload
    //ID3D11VertexShader* vertex_shader;
    //ID3D11PixelShader* pixel_shader;
};

/* initialize the renderer resources to render a scene */
GfxSceneResources scene_create_render_resource_for_scene(GfxState* gfx_state, ui::SceneDef* scene);

/* free the resources */
void scene_gfx_destroy(GfxSceneResources* gfx_resources);

#endif // T7LUAEDITOR_SCENE_H



/*
T7 LUIAnimationStateFlags

enum LUIAnimationStateFlags
{
    AS_LAYOUT_CACHED = 0x2,
    AS_STENCIL = 0x4,
    AS_FOCUS = 0x8,
    AS_LEFT_PX = 0x40,
    AS_LEFT_PT = 0x80,
    AS_ZOOM = 0x100,
    AS_XROT = 0x200,
    AS_YROT = 0x400,
    AS_ZROT = 0x800,
    AS_RED = 0x1000,
    AS_GREEN = 0x2000,
    AS_BLUE = 0x4000,
    AS_ALPHA = 0x8000,
    AS_USERDATA_FLOAT = 0x10000,
    AS_SHADERVECTOR_0 = 0x20000,
    AS_SHADERVECTOR_1 = 0x40000,
    AS_SHADERVECTOR_2 = 0x80000,
    AS_SHADERVECTOR_3 = 0x100000,
    AS_SHADERVECTOR_4 = 0x200000,
    AS_SHADERVECTOR_5 = 0x400000,
    AS_MATERIAL = 0x800000,
    AS_FONT = 0x1000000,
    AS_ALIGNMENT = 0x2000000,
    AS_UI3D_WINDOW = 0x4000000,
    AS_SCALE = 0x8000000,
    AS_USE_GAMETIME = 0x10000000,
    AS_TOP_PT = 0x20000000,
    AS_TOP_PX = 0x40000000,
    AS_RIGHT_PT = 0x80000000,
    AS_RIGHT_PX = 0x100000000,
    AS_BOTTOM_PT = 0x200000000,
    AS_BOTTOM_PX = 0x400000000,
    AS_LETTER_SPACING = 0x1000000000,
    AS_LINE_SPACING = 0x2000000000,
    AS_IMAGE = 0x4000000000,
};


struct __attribute__((aligned(8))) LUIElement
{
   LUIAnimationState currentAnimationState;
   LUIElement *parent;
   LUIElement *prevSibling;
   LUIElement *nextSibling;
   LUIElement *firstChild;
   LUIElement *lastChild;
   LUIElementLayoutFunction layoutFunction;
   LUIElementRenderFunction renderFunction;
   LUIElementMouseFunction mouseFunction;
   LUIElementCloseFunction closeFunction;
   LUIAnimationState *prevAnimationState;
   LUIAnimationState *nextAnimationState;
   int cacheRef;
   UIQuadCache *cache;
   int priority;
   int textRef;
   int animationTimeLeft;
   int animationDuration;
   int animationNameLuaRef;
   int strongLuaReference;
   float left;
   float top;
   float right;
   float bottom;
   LUIElement::$C4ABE194573AF402BCC947A60729C646 _anon_0;
   LUIElement::$3F3D4AB6D9209469EE6676535BC976BB _anon_1;
   LUIElement::$5DD91E8C5F43C9F95B3D72C7D7E9684A _anon_2;
   float textDimBottom;
   LUIElement::$D6C9215D993092FBE07E662A6FAC5A1A _anon_3;
   LUIElement::$A45ED59354BB6FB3558170006C28F313 _anon_4;
   UIModelIndex model;
};

struct __attribute__((aligned(8))) LUIAnimationState
{
    LUA_MATERIAL_DATATYPE material;
    TTFDef *font;
    uint64_t flags;
    int luaRef;
    unsigned __int8 tweenFlags;
    int ui3DWindow;
    float leftPct;
    float topPct;
    float rightPct;
    float bottomPct;
    float leftPx;
    float topPx;
    float rightPx;
    float bottomPx;
    float globalLeft;
    float globalTop;
    float globalRight;
    float globalBottom;
    float zoom;
    float xRot;
    float yRot;
    float zRot;
    float scale;
    float red;
    float green;
    float blue;
    float alpha;
    vec4_t shaderVector0;
    vec4_t shaderVector1;
    vec4_t shaderVector2;
    vec4_t shaderVector3;
    vec4_t shaderVector4;
    LUIAnimationState::$A3CFED264DE7974CE1B7F8FCF2520F75 _anon_0;
    LUIAnimationState::$2EA9413DFD814DAEF8A90F539116C751 _anon_1;
    LUIAlignment alignment;
    unsigned __int32 useGameTime : 1;
};
*/ 

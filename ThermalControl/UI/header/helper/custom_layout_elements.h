#ifndef CUSTOM_LAYOUT_ELEMENTS_H
#define CUSTOM_LAYOUT_ELEMENTS_H

/* Libraries */
#include "renderer/raylib.h"

/* Type definitions */
typedef enum
{
    CUSTOM_LAYOUT_ELEMENT_TYPE_3D_MODEL,
    CUSTOM_LAYOUT_ELEMENT_TYPE_GRAPH
} CustomLayoutElementType;

typedef struct
{
    Model model;
    float scale;
    Vector3 position;
    Matrix rotation;
} CustomLayoutElement_3DModel;

typedef struct
{
    void* graph_context;
} CustomLayoutElement_Graph;

typedef struct
{
    CustomLayoutElementType type;
    union {
        CustomLayoutElement_3DModel model;
        CustomLayoutElement_Graph graph;
    } customData;
} CustomLayoutElement;

#endif // CUSTOM_LAYOUT_ELEMENTS_H
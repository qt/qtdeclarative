vertexcolor_VSPS = $$PWD/vertexcolor.hlsl
vertexcolor_vshader.input = vertexcolor_VSPS
vertexcolor_vshader.header = vs_vertexcolor.hlslh
vertexcolor_vshader.entry = VS_VertexColor
vertexcolor_vshader.type = vs_5_0
vertexcolor_pshader.input = vertexcolor_VSPS
vertexcolor_pshader.header = ps_vertexcolor.hlslh
vertexcolor_pshader.entry = PS_VertexColor
vertexcolor_pshader.type = ps_5_0

stencilclip_VSPS = $$PWD/stencilclip.hlsl
stencilclip_vshader.input = stencilclip_VSPS
stencilclip_vshader.header = vs_stencilclip.hlslh
stencilclip_vshader.entry = VS_StencilClip
stencilclip_vshader.type = vs_5_0
stencilclip_pshader.input = stencilclip_VSPS
stencilclip_pshader.header = ps_stencilclip.hlslh
stencilclip_pshader.entry = PS_StencilClip
stencilclip_pshader.type = ps_5_0

HLSL_SHADERS = \
    vertexcolor_vshader vertexcolor_pshader \
    stencilclip_vshader stencilclip_pshader

load(hlsl_bytecode_header)

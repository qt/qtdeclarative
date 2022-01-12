qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o +qsb/test.vert test_rhi.vert

qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o +qsb/red.frag red_rhi.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o +qsb/test.frag test_rhi.frag

qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o +qsb/testprop.frag testprop.frag

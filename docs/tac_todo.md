# TAC TODO

---

## Create a material system


For vertex input layouts...
The shader *could* define the layout to use

at runtime, there will be a CBO with an iVtxBuffer
that the shader looks up bindlessly


The shader can specify, this is the vtx shd inputs i need,
the mesh can say these are the vtx shd inputs i have,
then it can magic between them.

so the vtx shader needs
- idx into vtx buffers
- stride
- attribute offsets

this is reinventing input layouts
hear me out - there could be an input layout buffer
it could use the same index as the iVtxBuf
```cpp
struct InputLayout
  struct InputLayoutElement
    int offset = -1
    int componentCount
    int componentType
  enum attribType{ pos, col, ..., count }
  attribs[ attribType::count ];
  int stride
```

then in the shader
v3 pos = GetInputElement( IEType::Pos, 3 )





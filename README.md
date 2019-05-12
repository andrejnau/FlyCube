# FlyCube

![sponza.png](screenshots/sponza.png)

* Engine
  * Rendering backend
      * DirectX 11
      * DirectX 12
      * Vulkan
      * OpenGL
  * Code generator for easy work constant buffers
  * Compile HLSL to DXBC, DXIL, SPIRV, GLSL
  * Automatically create resources binding by shader reflection
  * Directx 11 like API for all rendering backends
  * DirectX Raytracing
* Scene
  * Skeletal animation
  * Deferred rendering
  * Normal mapping
  * SSAO
  * Raytracing Ambient Occlusion
  * Point shadow mapping
  * PBR rendering
  * Tone mapping

## Engine API example
```cpp
program.UseProgram();

scene.ia.indices.Bind();
scene.ia.positions.BindToSlot(program.vs.ia.POSITION);
scene.ia.normals.BindToSlot(program.vs.ia.NORMAL);
scene.ia.tangents.BindToSlot(program.vs.ia.TANGENT);
scene.ia.texcoords.BindToSlot(program.vs.ia.TEXCOORD);

program.ps.om.rtv0.Attach(rtv);
program.ps.om.dsv.Attach(dsv);

program.vs.cbuffer.ConstantBuf.model = glm::transpose(model);
program.vs.cbuffer.ConstantBuf.view = glm::transpose(view);
program.vs.cbuffer.ConstantBuf.projection = glm::transpose(projection);

for (auto& range : scene.ia.ranges)
{
    auto& material = scene.GetMaterial(range.id);  
    program.ps.srv.normalMap.Attach(material.texture.normal);
    program.ps.srv.albedoMap.Attach(material.texture.albedo);
    program.ps.srv.roughnessMap.Attach(material.texture.roughness);    
    program.ps.srv.metalnessMap.Attach(material.texture.metalness);
    program.ps.srv.opacityMap.Attach(material.texture.opacity);

    context.DrawIndexed(range.index_count, range.start_index_location, range.base_vertex_location);
}
```

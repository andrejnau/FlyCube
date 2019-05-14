# FlyCube

FlyCube is C++ graphics API agnostic, simple Engine/Framework, Sponza viewer.

* Supported rendering backends
  * DirectX 11
  * DirectX 12 with DirectX Ray Tracing API
  * Vulkan
  * OpenGL

* Supported platforms
  * Windows 10 only

* Engine Features
  * HLSL as shading language for all backends
    * Compilation in DXBC, DXIL, SPIRV
    * Cross-compilation in GLSL with SPIRV-Cross
  * Generated shader helper by shader reflection
    * Easy to use resources binding
    * Constant buffers proxy for compile time access to members
  * Loading of images & 3D models based on gli, SOIL, assimp

![sponza.png](screenshots/sponza.png)

* Scene Features
  * Deferred rendering
  * Physically based rendering
  * Image based lighting
  * Ambient occlusion
    * Raytracing
    * Screen space
  * Normal mapping
  * Point shadow mapping
  * Skeletal animation
  * Multisample anti-aliasing
  * Tone mapping
  * Simple imgui based UI settings

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

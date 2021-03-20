// https://nvpro-samples.github.io/vk_mini_path_tracer/index.html
// https://github.com/nvpro-samples/vk_mini_path_tracer/blob/main/vk_mini_path_tracer/shaders/raytrace.comp.glsl

// Limit the kernel to trace at most 64 samples.
#define NUM_SAMPLES 64;

// Limit the kernel to trace at most 32 segments.
#define NUM_TRACED_SEGMENTS = 32;

RWTexture2D<float4> imageData;
RaytracingAccelerationStructure tlas;

[numthreads(8, 8, 1)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{   
    // The resolution of the buffer, which in this case is a hardcoded vector
    // of 2 unsigned integers:
    const uint2 resolution = uint2(512, 512);

    // Get the coordinates of the pixel for this invocation:
    //
    // .-------.-> x
    // |       |
    // |       |
    // '-------'
    // v
    // y
    const uint2 pixel = DTid.xy;

    // If the pixel is outside of the image, don't do anything:
    if((pixel.x >= resolution.x) || (pixel.y >= resolution.y))
    {
        return;
    }

    // State of the random number generator.
    uint rngState = resolution.x * pixel.y + pixel.x;  // Initial seed

    // This scene uses a right-handed coordinate system like the OBJ file format, where the
    // +x axis points right, the +y axis points up, and the -z axis points into the screen.
    // The camera is located at (-0.001, 1, 6).
    const float3 cameraOrigin = float3(-0.001f, 1.0f, 6.0f);


    // The sum of the colors of all of the samples.
    float3 summedPixelColor = (0.0f).xxx;


    // Rays always originate at the camera for now. In the future, they'll
    // bounce around the scene.
    float3 rayOrigin = cameraOrigin;
    // Compute the direction of the ray for this pixel. To do this, we first
    // transform the screen coordinates to look like this, where a is the
    // aspect ratio (width/height) of the screen:
    //           1
    //    .------+------.
    //    |      |      |
    // -a + ---- 0 ---- + a
    //    |      |      |
    //    '------+------'
    //          -1
    const float2 screenUV = float2((2.0 * float(pixel.x) + 1.0 - resolution.x) / resolution.y,    //
                                  -(2.0 * float(pixel.y) + 1.0 - resolution.y) / resolution.y);   // Flip the y axis

    // Next, define the field of view by the vertical slope of the topmost rays,
    // and create a ray direction:
    const float fovVerticalSlope = 1.0 / 5.0;
    float3 rayDirection = float3(fovVerticalSlope * screenUV.x, fovVerticalSlope * screenUV.y, -1.0);

    // Trace the ray and see if and where it intersects the scene!
    // First, initialize a ray query object:

    RayDesc rayDesc;
    rayDesc.Origin = rayOrigin;        // Ray origin
    rayDesc.Direction = rayDirection;  // Ray direction
    rayDesc.TMin = 0.001;              // Minimum t-value
    rayDesc.TMax = 10000.0;            // Maximum t-value

    RayQuery<RAY_FLAG_FORCE_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> rayQuery;
    rayQuery.TraceRayInline(tlas,                  // Top-level acceleration structure
                            RAY_FLAG_FORCE_OPAQUE, // Ray flags, https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#ray-flags
                            0xFF,                  // 8-bit instance mask, here saying "trace against all instances"
                            rayDesc);

    // Start traversal, and loop over all ray-scene intersections. When this finishes,
    // rayQuery stores a "committed" intersection, the closest intersection (if any).
    while(rayQuery.Proceed())
    {
    }

    // Get the t-value of the intersection (if there's no intersection, this will
    // be tMax = 10000.0). "true" says "get the committed intersection."
    //const float t = rayQueryGetIntersectionTEXT(rayQuery, true);
    const float t = rayQuery.CommittedRayT();


    // Give the pixel the color (t/10, t/10, t/10):
    imageData[pixel] = float4((t / 10.0).xxx, 1.0f);
}

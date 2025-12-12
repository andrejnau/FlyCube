struct VsOutput {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

static const uint kIndexCount = 64;
static const uint kRows = 4;
static const uint kColumns = 4;
static const float4 kPass = float4(0.0, 1.0, 0.0, 1.0);
static const float4 kFail = float4(1.0, 0.0, 0.0, 1.0);
static const float4 kIgnore = float4(1.0, 0.0, 1.0, 1.0);

uint MakeU32(uint index) {
    uint b0 = index * 4 + 0;
    uint b1 = index * 4 + 1;
    uint b2 = index * 4 + 2;
    uint b3 = index * 4 + 3;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

Buffer<uint> buffer_uint1;

float4 RunBufferTest1() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (buffer_uint1[u] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

Buffer<uint2> buffer_uint2;

float4 RunBufferTest2() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (buffer_uint2[u / 2][u % 2] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

float4 RunBufferTest3() {
    return kIgnore;
}

Buffer<uint4> buffer_uint4;

float4 RunBufferTest4() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (buffer_uint4[u / 4][u % 4] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

RWBuffer<uint> rwbuffer_uint1;

float4 RunRWBufferTest1() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (rwbuffer_uint1[u] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

RWBuffer<uint2> rwbuffer_uint2;

float4 RunRWBufferTest2() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (rwbuffer_uint2[u / 2][u % 2] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

float4 RunRWBufferTest3() {
    return kIgnore;
}

RWBuffer<uint4> rwbuffer_uint4;

float4 RunRWBufferTest4() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (rwbuffer_uint4[u / 4][u % 4] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

StructuredBuffer<uint> structured_buffer_uint1;

float4 RunStructuredBufferTest1() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (structured_buffer_uint1[u] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

StructuredBuffer<uint2> structured_buffer_uint2;

float4 RunStructuredBufferTest2() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (structured_buffer_uint2[u / 2][u % 2] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

StructuredBuffer<uint3> structured_buffer_uint3;

float4 RunStructuredBufferTest3() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (structured_buffer_uint3[u / 3][u % 3] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

StructuredBuffer<uint4> structured_buffer_uint4;

float4 RunStructuredBufferTest4() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (structured_buffer_uint4[u / 4][u % 4] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

RWStructuredBuffer<uint> rwstructured_buffer_uint1;

float4 RunRWStructuredBufferTest1() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (rwstructured_buffer_uint1[u] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

RWStructuredBuffer<uint2> rwstructured_buffer_uint2;

float4 RunRWStructuredBufferTest2() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (rwstructured_buffer_uint2[u / 2][u % 2] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

RWStructuredBuffer<uint3> rwstructured_buffer_uint3;

float4 RunRWStructuredBufferTest3() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (rwstructured_buffer_uint3[u / 3][u % 3] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

RWStructuredBuffer<uint4> rwstructured_buffer_uint4;

float4 RunRWStructuredBufferTest4() {
    for (uint u = 0; u < kIndexCount; ++u) {
        if (rwstructured_buffer_uint4[u / 4][u % 4] != MakeU32(u)) {
            return kFail;
        }
    }
    return kPass;
}

float4 RunTest(uint index) {
    switch (index) {
    case 0:
        return RunBufferTest1();
    case 1:
        return RunBufferTest2();
    case 2:
        return RunBufferTest3();
    case 3:
        return RunBufferTest4();
    case 4:
        return RunRWBufferTest1();
    case 5:
        return RunRWBufferTest2();
    case 6:
        return RunRWBufferTest3();
    case 7:
        return RunRWBufferTest4();
    case 8:
        return RunStructuredBufferTest1();
    case 9:
        return RunStructuredBufferTest2();
    case 10:
        return RunStructuredBufferTest3();
    case 11:
        return RunStructuredBufferTest4();
    case 12:
        return RunRWStructuredBufferTest1();
    case 13:
        return RunRWStructuredBufferTest2();
    case 14:
        return RunRWStructuredBufferTest3();
    case 15:
        return RunRWStructuredBufferTest4();
    default:
        return kIgnore;
    }
}

float4 main(VsOutput input) : SV_TARGET
{
    uint i = min(uint(input.uv.y * kRows), kRows - 1);
    uint j = min(uint(input.uv.x * kColumns), kColumns - 1);
    return RunTest(i * kColumns + j);
}

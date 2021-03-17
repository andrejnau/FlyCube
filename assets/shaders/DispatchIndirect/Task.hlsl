struct DispatchIndirectCommand
{
    uint thread_group_count_x;
    uint thread_group_count_y;
    uint thread_group_count_z;
};

RWStructuredBuffer<DispatchIndirectCommand> Argument;

[numthreads(1, 1, 1)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{
    Argument[DTid.x].thread_group_count_x = 64;
    Argument[DTid.y].thread_group_count_y = 64;
    Argument[DTid.z].thread_group_count_z = 1;
}
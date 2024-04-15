GlobalRootSignature GlobalRS = { "UAV(u0)" };
RWStructuredBuffer<uint> UAV : register(u0);

//!< ここでは CPU から以下のように 4 ディスパッチ 分コールされる
//constexpr std::array InputData = {
//  EntryRecord({ .GridSize = 1, .RecordIndex = 0 }),
//  EntryRecord({ .GridSize = 2, .RecordIndex = 1 }),
//  EntryRecord({ .GridSize = 3, .RecordIndex = 2 }),
//  EntryRecord({ .GridSize = 4, .RecordIndex = 3 }),
//};
//!< システム変数 SV_DispatchGrid が GridSize に指定されている為、それぞれのディスパッチのグリッドサイズは 1, 2, 3, 4 となる
struct EntryRecord
{
    uint GridSize : SV_DispatchGrid;
    uint RecordIndex;
};

struct SecondNodeInput
{
    uint RecordIndex;
    uint IncrementValue;
};

struct ThirdNodeInput
{
    uint RecordIndex;
};

static const uint NumEntryRecords = 4;

// GridSize = 1, NumThreads = 2
// [0] = 0 * 2 + 0 + 1 = 1
// 
// [0] = 1 * 2 + 1 + 1 = 4
// ----
// GridSize = 2, NumThreads = 2
// [1] = 0 * 2 + 0 + 1 = 1
// [1] = 1 * 2 + 0 + 1 = 3
// 
// [1] = 2 * 2 + 1 + 1 = 6
// [1] = 3 * 2 + 1 + 1 = 8
// -----
// GridSize = 3, NumThreads = 2
// [2] = 0 * 2 + 0 + 1 = 1
// [2] = 1 * 2 + 0 + 1 = 3
// [2] = 2 * 2 + 0 + 1 = 5
//
// [2] = 3 * 2 + 1 + 1 = 8
// [2] = 4 * 2 + 1 + 1 = 10
// [2] = 5 * 2 + 1 + 1 = 12
// -----
// GridSize = 4, NumThreads = 2
// [3] = 0 * 2 + 0 + 1 = 1
// [3] = 1 * 2 + 0 + 1 = 3
// [3] = 2 * 2 + 0 + 1 = 5
// [3] = 3 * 2 + 0 + 1 = 7
//
// [3] = 4 * 2 + 1 + 1 = 10
// [3] = 5 * 2 + 1 + 1 = 12
// [3] = 6 * 2 + 1 + 1 = 14
// [3] = 7 * 2 + 1 + 1 = 16
[Shader("node")]
[NodeLaunch("broadcasting")]
//!< NodeMaxDispatchGrid を使用する場合、システム変数 SV_DispatchGrid でグリッドサイズを指定する必要がある (ここで指定しているのは最大値)
[NodeMaxDispatchGrid(16, 1, 1)]
[NumThreads(2, 1, 1)]
void FirstNode(
    DispatchNodeInputRecord<EntryRecord> InputData,
    [MaxRecords(2)] NodeOutput<SecondNodeInput> SecondNode,
    uint ThreadIndex : SV_GroupIndex,
    uint DispatchThreadID : SV_DispatchThreadID)
{
    GroupNodeOutputRecords<SecondNodeInput> Out = SecondNode.GetGroupNodeOutputRecords(2);

    Out[ThreadIndex].RecordIndex = InputData.Get().RecordIndex;
    Out[ThreadIndex].IncrementValue = DispatchThreadID * 2 + ThreadIndex + 1;
    Out.OutputComplete();
}

[Shader("node")]
[NodeLaunch("broadcasting")]
//!< NodeDispatchGrid を使用する場合、ここで指定したグリッドサイズで起動される
[NodeDispatchGrid(2, 1, 1)]
[NumThreads(2, 1, 1)]
//!< 指定が無い場合は関数名になるので大抵は不要、エントリポイントインデックスを指定する場合は明示的に使用する必要がある
//!< DispatchGraph() 時に EntrypointIndex = 1 を指定するとこちらが使用される
[NodeID("FirstNode", 1)]
void FirstNode1(
    DispatchNodeInputRecord<EntryRecord> InputData,
    [MaxRecords(2)] NodeOutput<SecondNodeInput> SecondNode,
    uint ThreadIndex : SV_GroupIndex,
    uint DispatchThreadID : SV_DispatchThreadID)
{
    GroupNodeOutputRecords<SecondNodeInput> Out = SecondNode.GetGroupNodeOutputRecords(2);

    Out[ThreadIndex].RecordIndex = InputData.Get().RecordIndex;
    Out[ThreadIndex].IncrementValue = DispatchThreadID * 2 + ThreadIndex + 1;
    Out.OutputComplete();
}

// UAV[0] = 1 + 4 = 5(0x5)
// UAV[1] = 1 + 3 + 6 + 8 = 18(0x12) 
// UAV[2] = 1 + 3 + 5 + 8 + 10 + 12 = 39(0x27)
// UAV[3] = 1 + 3 + 5 + 7 + 10 + 12 + 14 + 16 = 68(0x44)
[Shader("node")]
[NodeLaunch("thread")]
void SecondNode(
    ThreadNodeInputRecord<SecondNodeInput> InputData,
    [MaxRecords(1)] NodeOutput<ThirdNodeInput> ThirdNode)
{
    InterlockedAdd(UAV[InputData.Get().RecordIndex], InputData.Get().IncrementValue);

    ThreadNodeOutputRecords<ThirdNodeInput> Out = ThirdNode.GetThreadNodeOutputRecords(1);
    Out.Get().RecordIndex = InputData.Get().RecordIndex;
    Out.OutputComplete();
}

groupshared uint Sum[NumEntryRecords];

// 起動されたスレッド数になる
// UAV[4] = 2
// UAV[5] = 4
// UAV[6] = 6
// UAV[7] = 8
[Shader("node")]
[NodeLaunch("coalescing")]
[NumThreads(32, 1, 1)]
void ThirdNode(
    [MaxRecords(32)] GroupNodeInputRecords<ThirdNodeInput> InputData,
    uint ThreadIndex : SV_GroupIndex)
{
    if (ThreadIndex >= InputData.Count()) {
        return;
    }

    for (uint i = 0; i < NumEntryRecords; i++) {
        Sum[i] = 0;
    }

    Barrier(GROUP_SHARED_MEMORY, GROUP_SCOPE | GROUP_SYNC);
    {
        InterlockedAdd(Sum[InputData[ThreadIndex].RecordIndex], 1);
    }
    Barrier(GROUP_SHARED_MEMORY, GROUP_SCOPE | GROUP_SYNC);

    //!< 0 番のスレッドが集計するので、他のスレッドはここまで
    if (ThreadIndex > 0) {
        return;
    }

    for (uint i = 0; i < NumEntryRecords; i++) {
        UAV[NumEntryRecords + i] = Sum[i];
    }
}
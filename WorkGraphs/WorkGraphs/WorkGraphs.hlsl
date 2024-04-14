GlobalRootSignature GlobalRS = { "UAV(u0)" };
RWStructuredBuffer<uint> UAV : register(u0);

//!< �����ł� CPU ����ȉ��̂悤�� 4 �f�B�X�p�b�` ���R�[�������
//constexpr std::array InputData = {
//  EntryRecord({ .GridSize = 1, .RecordIndex = 0 }),
//  EntryRecord({ .GridSize = 2, .RecordIndex = 1 }),
//  EntryRecord({ .GridSize = 3, .RecordIndex = 2 }),
//  EntryRecord({ .GridSize = 4, .RecordIndex = 3 }),
//};
//!< �V�X�e���ϐ� SV_DispatchGrid �� GridSize �Ɏw�肳��Ă���ׁA���ꂼ��̃f�B�X�p�b�`�̃O���b�h�T�C�Y�� 1, 2, 3, 4 �ƂȂ�
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

static const uint c_numEntryRecords = 4;

// [NumThreads(2, 1, 1)] �Ȃ̂ňȉ��̂悤�ɂȂ�
// 
// UAV[0] = 0 * 2 + 0 + 1 = 1
// 
// UAV[0] = 1 * 2 + 1 + 1 = 4
// ----
// UAV[1] = 0 * 2 + 0 + 1 = 1
// UAV[1] = 1 * 2 + 0 + 1 = 3
// 
// UAV[1] = 2 * 2 + 1 + 1 = 6
// UAV[1] = 3 * 2 + 1 + 1 = 8
// -----
// UAV[2] = 0 * 2 + 0 + 1 = 1
// UAV[2] = 1 * 2 + 0 + 1 = 3
// UAV[2] = 2 * 2 + 0 + 1 = 5
//
// UAV[2] = 3 * 2 + 1 + 1 = 8
// UAV[2] = 4 * 2 + 1 + 1 = 10
// UAV[2] = 5 * 2 + 1 + 1 = 12
// -----
// UAV[3] = 0 * 2 + 0 + 1 = 1
// UAV[3] = 1 * 2 + 0 + 1 = 3
// UAV[3] = 2 * 2 + 0 + 1 = 5
// UAV[3] = 3 * 2 + 0 + 1 = 7
//
// UAV[3] = 4 * 2 + 1 + 1 = 10
// UAV[3] = 5 * 2 + 1 + 1 = 12
// UAV[3] = 6 * 2 + 1 + 1 = 14
// UAV[3] = 7 * 2 + 1 + 1 = 16
[Shader("node")]
[NodeLaunch("broadcasting")]
[NodeMaxDispatchGrid(16, 1, 1)]
[NumThreads(2, 1, 1)]
void RirstNode(
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

groupshared uint g_sum[c_numEntryRecords];

// �N�����ꂽ�X���b�h���ɂȂ�
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

    for (uint i = 0; i < c_numEntryRecords; i++) {
        g_sum[i] = 0;
    }

    Barrier(GROUP_SHARED_MEMORY, GROUP_SCOPE | GROUP_SYNC);
    {
        InterlockedAdd(g_sum[InputData[ThreadIndex].RecordIndex], 1);
    }
    Barrier(GROUP_SHARED_MEMORY, GROUP_SCOPE | GROUP_SYNC);

    //!< 0 �Ԃ̃X���b�h���W�v����̂ŁA���̃X���b�h�͂����܂�
    if (ThreadIndex > 0) {
        return;
    }

    for (uint i = 0; i < c_numEntryRecords; i++) {
        UAV[c_numEntryRecords + i] = g_sum[i];
    }
}
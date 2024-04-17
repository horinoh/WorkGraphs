GlobalRootSignature GlobalRS = { "UAV(u0)" };
RWStructuredBuffer<uint> UAV : register(u0);

LocalRootSignature LocalRS = { "RootConstants(num32BitConstants = 1, b0)" };
struct LocalRootArgs { uint Value; };
ConstantBuffer<LocalRootArgs> NodeConstants : register(b0);

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

//!< �Z�J���h�m�[�h���G���g���|�C���g(�z��̓Y����)�ŌĂѕ�����
//#define USE_ARRAY
[Shader("node")]
[NodeLaunch("broadcasting")]
//!< NodeMaxDispatchGrid ���g�p����ꍇ�A�V�X�e���ϐ� SV_DispatchGrid �ŃO���b�h�T�C�Y���w�肷��K�v������ (�����Ŏw�肵�Ă���͍̂ő�l)
[NodeMaxDispatchGrid(16, 1, 1)]
[NumThreads(2, 1, 1)]
void FirstNode(
    DispatchNodeInputRecord<EntryRecord> InputData,
#ifdef USE_ARRAY
    //!< �z��Ƃ��Đ錾 (NodeArraySize(), NodeOutputArray)
    [MaxRecords(2)][NodeArraySize(2)] NodeOutputArray<SecondNodeInput> SecondNode,
#else
    [MaxRecords(2)] NodeOutput<SecondNodeInput> SecondNode,
#endif
    uint ThreadIndex : SV_GroupIndex,
    uint DispatchThreadID : SV_DispatchThreadID)
{
#ifdef USE_ARRAY
    //!< �Z�J���h�m�[�h[1] ��I��
    GroupNodeOutputRecords<SecondNodeInput> Out = SecondNode[1].GetGroupNodeOutputRecords(2);
#else
    GroupNodeOutputRecords<SecondNodeInput> Out = SecondNode.GetGroupNodeOutputRecords(2);
#endif

    Out[ThreadIndex].RecordIndex = InputData.Get().RecordIndex;
    Out[ThreadIndex].IncrementValue = DispatchThreadID * 2 + ThreadIndex + 1;
    Out.OutputComplete();
}

[Shader("node")]
[NodeLaunch("broadcasting")]
//!< NodeDispatchGrid ���g�p����ꍇ�A�����Ŏw�肵���O���b�h�T�C�Y�ŋN�������
[NodeDispatchGrid(2, 1, 1)]
[NumThreads(2, 1, 1)]
//!< NodeID �w�肪�����ꍇ�͊֐������g����̂ő��͕s�v�A�G���g���|�C���g���w�莞�͖����I�Ɏg�p (DispatchGraph() ���� EntrypointIndex = 1 �w�肷��Ǝg�p�����)
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

// UAV[0] = (1 + 4) * 2 = 10
// UAV[1] = (1 + 3 + 6 + 8) * 2 = 36 
// UAV[2] = (1 + 3 + 5 + 8 + 10 + 12) * 2 = 78
// UAV[3] = (1 + 3 + 5 + 7 + 10 + 12 + 14 + 16) * 2 = 136
//!< �Z�J���h�m�[�h[1]
[NodeID("SecondNode", 1)]
[Shader("node")]
[NodeLaunch("thread")]
void SecondNode1(
    ThreadNodeInputRecord<SecondNodeInput> InputData,
    [MaxRecords(1)] NodeOutput<ThirdNodeInput> ThirdNode)
{
    InterlockedAdd(UAV[InputData.Get().RecordIndex], InputData.Get().IncrementValue * 2);

    ThreadNodeOutputRecords<ThirdNodeInput> Out = ThirdNode.GetThreadNodeOutputRecords(1);
    Out.Get().RecordIndex = InputData.Get().RecordIndex;
    Out.OutputComplete();
}

groupshared uint Sum[NumEntryRecords];

//#define USE_LOCAL_ROOT
[Shader("node")]
[NodeLaunch("coalescing")]
[NumThreads(32, 1, 1)]
//[NodeMaxRecursionDepth(3)]
#ifdef USE_LOCAL_ROOT
//!< ���[�J�����[�g�e�[�u���̃C���f�b�N�X�𖾎��I�Ɏw�肷��ꍇ�Ɏg�p (�w�肵�Ȃ��ꍇ�͎����I�Ɋ���U����)
//[NodeLocalRootArgumentsTableIndex(2)]
#endif
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
#ifdef USE_LOCAL_ROOT
        //!< NodeConstants.Value == 2 �̈�
        // UAV[4] = 4
        // UAV[5] = 8
        // UAV[6] = 12
        // UAV[7] = 16
        InterlockedAdd(Sum[InputData[ThreadIndex].RecordIndex], NodeConstants.Value);
#else
        // UAV[4] = 2
        // UAV[5] = 4
        // UAV[6] = 6
        // UAV[7] = 8
        InterlockedAdd(Sum[InputData[ThreadIndex].RecordIndex], 1);
#endif
    }
    Barrier(GROUP_SHARED_MEMORY, GROUP_SCOPE | GROUP_SYNC);

    //if (GetRemainingRecursionLevels()) {}

    //!< 0 �Ԃ̃X���b�h���W�v����̂ŁA���̃X���b�h�͂����܂�
    if (ThreadIndex > 0) {
        return;
    }

    for (uint i = 0; i < NumEntryRecords; i++) {
        UAV[NumEntryRecords + i] = Sum[i];
    }
}
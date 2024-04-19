//!< �Z�J���h�m�[�h���G���g���|�C���g(�z��̓Y����)�ŌĂѕ����� (FirstNode1 ���g�p�����)
//#define USE_ARRAY
//!< ���[�J�����[�g�V�O�l�`�� (NodeConstants.Value) ���g�p
//#define USE_LOCAL_ROOT
//!< �ċA�Ăяo�� 
#define USE_RECURSION

//!< �O���[�o�����[�g�V�O�l�`��
GlobalRootSignature GlobalRS = { "UAV(u0)" };
RWStructuredBuffer<uint> UAV : register(u0);

//!< ���[�J�����[�g�V�O�l�`�� (�m�[�h��)
LocalRootSignature LocalRS = { "RootConstants(num32BitConstants = 1, b0)" };
struct LocalRootArgs { uint Value; };
ConstantBuffer<LocalRootArgs> NodeConstants : register(b0);

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

//!< �����ł� CPU ����ȉ��̂悤�� 4 �f�B�X�p�b�` ���R�[�������
//constexpr std::array InputData = {
//  EntryRecord({ .GridSize = 1, .RecordIndex = 0 }),
//  EntryRecord({ .GridSize = 2, .RecordIndex = 1 }),
//  EntryRecord({ .GridSize = 3, .RecordIndex = 2 }),
//  EntryRecord({ .GridSize = 4, .RecordIndex = 3 }),
//};
//!< [NodeMaxDispatchGrid()] �g�p���́A�V�X�e���ϐ� GridSize : SV_DispatchGrid �Ŏw�肳���O���b�h�T�C�Y�ƂȂ�A�����ł� 1, 2, 3, 4 �ƂȂ� (NodeMaxDispatchGrid �͍ő�l�̎w��)
//!< [NodeDispatchGrid()] �g�p���́ANodeDispatchGrid �����Ŏw�肵���O���b�h�T�C�Y�ƂȂ� (SV_DispatchGrid �͎Q�Ƃ���Ȃ�)

// GridSize = 1, 2, 3, 4, NumThreads = 2
// RecordIndex = 0, IncrementValue = 0 * 2 + 0 + 1 = 1
// 
// RecordIndex = 0, IncrementValue = 1 * 2 + 1 + 1 = 4
// ----
// RecordIndex = 1, IncrementValue = 0 * 2 + 0 + 1 = 1
// RecordIndex = 1, IncrementValue = 1 * 2 + 0 + 1 = 3
// 
// RecordIndex = 1, IncrementValue = 2 * 2 + 1 + 1 = 6
// RecordIndex = 1, IncrementValue = 3 * 2 + 1 + 1 = 8
// -----
// RecordIndex = 2, IncrementValue = 0 * 2 + 0 + 1 = 1
// RecordIndex = 2, IncrementValue = 1 * 2 + 0 + 1 = 3
// RecordIndex = 2, IncrementValue = 2 * 2 + 0 + 1 = 5
//
// RecordIndex = 2, IncrementValue = 3 * 2 + 1 + 1 = 8
// RecordIndex = 2, IncrementValue = 4 * 2 + 1 + 1 = 10
// RecordIndex = 2, IncrementValue = 5 * 2 + 1 + 1 = 12
// -----
// RecordIndex = 3, IncrementValue = 0 * 2 + 0 + 1 = 1
// RecordIndex = 3, IncrementValue = 1 * 2 + 0 + 1 = 3
// RecordIndex = 3, IncrementValue = 2 * 2 + 0 + 1 = 5
// RecordIndex = 3, IncrementValue = 3 * 2 + 0 + 1 = 7
//
// RecordIndex = 3, IncrementValue = 4 * 2 + 1 + 1 = 10
// RecordIndex = 3, IncrementValue = 5 * 2 + 1 + 1 = 12
// RecordIndex = 3, IncrementValue = 6 * 2 + 1 + 1 = 14
// RecordIndex = 3, IncrementValue = 7 * 2 + 1 + 1 = 16
[Shader("node")]
[NodeLaunch("broadcasting")]
//!< (�����Ŏw�肵�Ă���͍̂ő�l�ł���) �V�X�e���ϐ� SV_DispatchGrid �ɂ��O���b�h�T�C�Y���w�肷��K�v������ 
[NodeMaxDispatchGrid(16, 1, 1)]
[NumThreads(2, 1, 1)]
void FirstNode(
    DispatchNodeInputRecord<EntryRecord> InputData,
#ifdef USE_ARRAY
    //!< �z��Ƃ��Đ錾
    [MaxRecords(2)][NodeArraySize(2)] NodeOutputArray<SecondNodeInput> SecondNode,
#else
    [MaxRecords(2)] NodeOutput<SecondNodeInput> SecondNode,
#endif
    uint ThreadIndex : SV_GroupIndex,
    uint DispatchThreadID : SV_DispatchThreadID)
{
#ifdef USE_ARRAY
    //!< SecondNode1 ��I��
    GroupNodeOutputRecords<SecondNodeInput> Out = SecondNode[1].GetGroupNodeOutputRecords(2);
#else
    GroupNodeOutputRecords<SecondNodeInput> Out = SecondNode.GetGroupNodeOutputRecords(2);
#endif
    Out[ThreadIndex].RecordIndex = InputData.Get().RecordIndex;
    Out[ThreadIndex].IncrementValue = DispatchThreadID * 2 + ThreadIndex + 1;
    Out.OutputComplete();
}

// GridSize = 2, NumThreads = 2
// RecordIndex = 0, IncrementValue = 0 * 2 + 0 + 1 = 1
// RecordIndex = 0, IncrementValue = 1 * 2 + 0 + 1 = 3
// 
// RecordIndex = 0, IncrementValue = 2 * 2 + 1 + 1 = 6
// RecordIndex = 0, IncrementValue = 3 * 2 + 1 + 1 = 8
// ----
// RecordIndex = 1, IncrementValue = 0 * 2 + 0 + 1 = 1
// RecordIndex = 1, IncrementValue = 1 * 2 + 0 + 1 = 3
// 
// RecordIndex = 1, IncrementValue = 2 * 2 + 1 + 1 = 6
// RecordIndex = 1, IncrementValue = 3 * 2 + 1 + 1 = 8
// ----
// RecordIndex = 2, IncrementValue = 0 * 2 + 0 + 1 = 1
// RecordIndex = 2, IncrementValue = 1 * 2 + 0 + 1 = 3
// 
// RecordIndex = 2, IncrementValue = 2 * 2 + 1 + 1 = 6
// RecordIndex = 2, IncrementValue = 3 * 2 + 1 + 1 = 8
// ----
// RecordIndex = 3, IncrementValue = 0 * 2 + 0 + 1 = 1
// RecordIndex = 3, IncrementValue = 1 * 2 + 0 + 1 = 3
// 
// RecordIndex = 3, IncrementValue = 2 * 2 + 1 + 1 = 6
// RecordIndex = 3, IncrementValue = 3 * 2 + 1 + 1 = 8
// ----
[Shader("node")]
[NodeLaunch("broadcasting")]
//!< NodeDispatchGrid ���g�p����ꍇ�́A�����Ŏw�肵���O���b�h�T�C�Y�ŋN�������
[NodeDispatchGrid(2, 1, 1)]
[NumThreads(2, 1, 1)]
//!< �w�肪�����ꍇ�͊֐����ƂȂ�̂ő��̃P�[�X�ł͕s�v�A�G���g���|�C���g���w�肷��ꍇ�͖����I�Ɏw�肷��K�v������ (DispatchGraph() ���� EntrypointIndex = N �̂悤�Ɏw�肷��)
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

//!< FirstNode ���痈���ꍇ (GridSize = 1, 2, 3, 4, NumThreads = 2)
// UAV[0] = 1 + 4 = 5(0x5)
// UAV[1] = 1 + 3 + 6 + 8 = 18(0x12) 
// UAV[2] = 1 + 3 + 5 + 8 + 10 + 12 = 39(0x27)
// UAV[3] = 1 + 3 + 5 + 7 + 10 + 12 + 14 + 16 = 68(0x44)
// 
//!< FirstNode1 ���痈���ꍇ (GridSize = 2, NumThreads = 2)
// UAV[0] = 1 + 3 + 6 + 8 = 18(0x12) 
// UAV[1] = 1 + 3 + 6 + 8 = 18(0x12) 
// UAV[2] = 1 + 3 + 6 + 8 = 18(0x12) 
// UAV[3] = 1 + 3 + 6 + 8 = 18(0x12) 
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

//!< FirstNode ���痈���ꍇ (FirstNode1 ���痈�邱�Ƃ͂Ȃ�)
// UAV[0] = 5 * 2 = 10
// UAV[1] = 18 * 2 = 36 
// UAV[2] = 39 * 2 = 78
// UAV[3] = 68 * 2 = 136
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

[Shader("node")]
[NodeLaunch("coalescing")]
[NumThreads(32, 1, 1)]
#ifdef USE_LOCAL_ROOT
//!< ���[�J�����[�g�e�[�u���̃C���f�b�N�X�𖾎��I�Ɏw�肷��ꍇ�Ɏg�p (�w�肵�Ȃ��ꍇ�͎����I�Ɋ���U����)
//[NodeLocalRootArgumentsTableIndex(2)]
#endif
#ifdef USE_RECURSION
//!< �ċA�Ăяo���A����ƍ��킹�� 1 + 3 = 4 ��R�[������鎖�ɂȂ�
[NodeMaxRecursionDepth(3)]
#endif
void ThirdNode(
    [MaxRecords(32)] GroupNodeInputRecords<ThirdNodeInput> InputData,
#ifdef USE_RECURSION
    [MaxRecords(1)] NodeOutput<ThirdNodeInput> ThirdNode,
#endif
    uint ThreadIndex : SV_GroupIndex)
{
    //!< ���̓f�[�^ 20 �ł���͂��Ȃ̂ŁA�����镪�̃X���b�h�͑����I��
    if (ThreadIndex >= InputData.Count()) {
        return;
    }

    //!< ���L��������ΏہA�X�R�[�v�̓O���[�v�A�O���[�v�𓯊�
    Barrier(GROUP_SHARED_MEMORY, GROUP_SCOPE | GROUP_SYNC);
    {
#ifdef USE_LOCAL_ROOT
        //!< NodeConstants.Value == 2 �̈�
        // UAV[4] = 2 + 2 = 4
        // UAV[5] = 2 + 2 + 2 + 2 = 8
        // UAV[6] = 2 + 2 + 2 + ... = 12
        // UAV[7] = 2 + 2 + 2 + ... = 16
        InterlockedAdd(UAV[InputData[ThreadIndex].RecordIndex + NumEntryRecords], NodeConstants.Value);
#else
        // UAV[4] = 1 + 1 = 2
        // UAV[5] = 1 + 1 + 1 + 1 = 4
        // UAV[6] = 1 + 1 + 1 + ... = 6
        // UAV[7] = 1 + 1 + 1 + ... = 8
        InterlockedAdd(UAV[InputData[ThreadIndex].RecordIndex + NumEntryRecords], 1);
#endif
    }
    Barrier(GROUP_SHARED_MEMORY, GROUP_SCOPE | GROUP_SYNC);

#ifdef USE_RECURSION
    // UAV[4] = 2 * (1 + 3) = 8
    // UAV[5] = 4 * (1 + 3) = 16
    // UAV[6] = 6 * (1 + 3) = 24
    // UAV[7] = 8 * (1 + 3) = 32
    if (GetRemainingRecursionLevels()) {
        ThreadNodeOutputRecords<ThirdNodeInput> Out = ThirdNode.GetThreadNodeOutputRecords(1);
        Out.Get().RecordIndex = InputData[ThreadIndex].RecordIndex;
        Out.OutputComplete();
    }
#endif
}
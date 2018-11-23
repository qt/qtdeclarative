/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4MI_P_H
#define QV4MI_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4global_p.h>
#include <private/qv4ir_p.h>
#include <private/qv4node_p.h>
#include <private/qv4operation_p.h>

#include <llvm/ADT/iterator.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

// This file contains the Machine Interface (MI) data structures, on which ultimately the assembler
// will operate:

class MIFunction; // containing all basic blocks, and a reference to the IR function

class MIBlock;    // containing an ordered sequence of instructions

class MIInstr;    // containing operands, and a reference to the IR node, that indicates which
                  // operation is represented by an instruction

class MIOperand;  // contains a description of where to get/put the input/result of an operation

// A detail about the stack slots: there two stacks, the JS stack and the native stack. A frame on
// the native stack is divided in two parts: the quad-word part and the double-word part. The
// qword part holds 64bit values, like doubles, and pointers on 64bit architectures. The dword part
// holds 32bit values, like int32s, booleans, and pointers on 32bit architectures. We need to know
// the type of value a slot holds, because if we have to move it to the JS stack, we have to box it
// correctly.
class MIOperand final
{
public:
    enum Kind {
        Invalid = 0,
        Constant,
        VirtualRegister,

        EngineRegister,
        CppFrameRegister,
        Function,

        JSStackSlot,
        BoolStackSlot,

        JumpTarget,
    };

    using List = QQmlJS::FixedPoolArray<MIOperand>;

public:
    MIOperand() = default;

    static MIOperand createConstant(Node *irNode)
    {
        MIOperand op;
        op.m_kind = Constant;
        op.m_irNode = irNode;
        return op;
    }

    static MIOperand createVirtualRegister(Node *irNode, unsigned vreg)
    {
        MIOperand op;
        op.m_kind = VirtualRegister;
        op.m_irNode = irNode;
        op.m_vreg = vreg;
        return op;
    }

    static MIOperand createEngineRegister(Node *irNode)
    {
        MIOperand op;
        op.m_kind = EngineRegister;
        op.m_irNode = irNode;
        return op;
    }

    static MIOperand createCppFrameRegister(Node *irNode)
    {
        MIOperand op;
        op.m_kind = CppFrameRegister;
        op.m_irNode = irNode;
        return op;
    }

    static MIOperand createFunction(Node *irNode)
    {
        MIOperand op;
        op.m_kind = Function;
        op.m_irNode = irNode;
        return op;
    }

    static MIOperand createJSStackSlot(Node *irNode, unsigned slot)
    {
        MIOperand op;
        op.m_kind = JSStackSlot;
        op.m_irNode = irNode;
        op.m_slot = slot;
        return op;
    }

    static MIOperand createBoolStackSlot(Node *irNode, unsigned slot)
    {
        MIOperand op;
        op.m_kind = BoolStackSlot;
        op.m_irNode = irNode;
        op.m_slot = slot;
        return op;
    }

    //### or name this createDeoptBlock?
    static MIOperand createJumpTarget(Node *irNode, MIBlock *targetBlock)
    {
        MIOperand op;
        op.m_kind = JumpTarget;
        op.m_irNode = irNode;
        op.m_targetBlock = targetBlock;
        return op;
    }

    Kind kind() const
    { return m_kind; }

    bool isValid() const
    { return m_kind != Invalid; }

    bool isConstant() const
    { return m_kind == Constant; }

    bool isVirtualRegister() const
    { return kind() == VirtualRegister; }

    bool isEngineRegister() const
    { return kind() == EngineRegister; }

    bool isCppFrameRegister() const
    { return kind() == CppFrameRegister; }

    bool isFunction() const
    { return kind() == Function; }

    bool isJSStackSlot() const
    { return kind() == JSStackSlot; }

    bool isBoolStackSlot() const
    { return kind() == BoolStackSlot; }

    bool isStackSlot() const
    { return isJSStackSlot() || isDWordSlot() || isQWordSlot(); }

    bool isJumpTarget() const
    { return kind() == JumpTarget; }

    Node *irNode() const
    { return m_irNode; }

    inline Type nodeType(MIFunction *f) const;

    QString debugString() const;

    QV4::Value constantValue() const
    {
        Q_ASSERT(isConstant());
        if (irNode()->opcode() == Meta::Undefined)
            return QV4::Value::undefinedValue();
        if (irNode()->opcode() == Meta::Empty)
            return QV4::Value::emptyValue();
        return ConstantPayload::get(*irNode()->operation())->value();
    }

    unsigned virtualRegister() const
    { Q_ASSERT(isVirtualRegister()); return m_vreg; }

    unsigned stackSlot() const
    { Q_ASSERT(isStackSlot()); return m_slot; }

    MIBlock *targetBlock() const
    { Q_ASSERT(isJumpTarget()); return m_targetBlock; }

    inline bool operator==(const MIOperand &other) const
    {
        if (kind() != other.kind())
            return false;

        if (isStackSlot())
            return stackSlot() == other.stackSlot();

        switch (kind()) {
        case MIOperand::Invalid:
            return !other.isValid();
        case MIOperand::Constant:
            return constantValue().asReturnedValue() == other.constantValue().asReturnedValue();
        case MIOperand::VirtualRegister:
            return virtualRegister() == other.virtualRegister();
        case MIOperand::JumpTarget:
            return targetBlock() == other.targetBlock();
        default:
            Q_UNREACHABLE();
            return false;
        }
    }

    bool isDWordSlot() const
    {
        switch (kind()) {
        case BoolStackSlot:
            return true;
        default:
            return false;
        }
    }

    bool isQWordSlot() const
    {
        switch (kind()) {
            //### TODO: double slots
        default:
            return false;
        }
    }

    bool overlaps(const MIOperand &other) const
    {
        if ((isDWordSlot() && other.isDWordSlot()) || (isQWordSlot() && other.isQWordSlot()))
            ; // fine, these are the same
        else if (kind() != other.kind())
            return false;

        if (isStackSlot())
            return stackSlot() == other.stackSlot();

        return false;
    }

private:
    Node *m_irNode = nullptr;
    union {
        unsigned m_vreg;
        unsigned m_slot;
        MIBlock *m_targetBlock = nullptr;
    };
    Kind m_kind = Invalid;
};

template <typename NodeTy> struct MIInstrListParentType {};
template <> struct MIInstrListParentType<MIInstr> { using type = MIBlock; };

template <typename NodeTy> class MIInstrList;

template <typename MISubClass>
class MIInstrListTraits : public llvm::ilist_noalloc_traits<MISubClass>
{
protected:
    using ListTy = MIInstrList<MISubClass>;
    using iterator = typename llvm::simple_ilist<MISubClass>::iterator;
    using ItemParentClass = typename MIInstrListParentType<MISubClass>::type;

public:
    MIInstrListTraits() = default;

protected:
    void setListOwner(ItemParentClass *listOwner)
    { m_owner = listOwner; }

private:
    ItemParentClass *m_owner = nullptr;

    /// getListOwner - Return the object that owns this list.  If this is a list
    /// of instructions, it returns the BasicBlock that owns them.
    ItemParentClass *getListOwner() const {
        return m_owner;
    }

    static ListTy &getList(ItemParentClass *Par) {
        return Par->*(Par->getSublistAccess());
    }

    static MIInstrListTraits<MISubClass> *getSymTab(ItemParentClass *Par) {
        return Par ? toPtr(Par->getValueSymbolTable()) : nullptr;
    }

public:
    void addNodeToList(MISubClass *V) { V->setParent(getListOwner()); }
    void removeNodeFromList(MISubClass *V) { V->setParent(nullptr); }
    void transferNodesFromList(MIInstrListTraits &L2, iterator first,
                               iterator last);
};

template <class T>
class MIInstrList: public llvm::iplist_impl<llvm::simple_ilist<T>, MIInstrListTraits<T>>
{
public:
    MIInstrList(typename MIInstrListTraits<T>::ItemParentClass *owner)
    { this->setListOwner(owner); }
};

class MIInstr final : public llvm::ilist_node_with_parent<MIInstr, MIBlock>
{
    Q_DISABLE_COPY_MOVE(MIInstr) // heap use only!

protected:
    friend class QQmlJS::MemoryPool;
    MIInstr() : m_operands(nullptr, 0) {}

    explicit MIInstr(Node *irNode, QQmlJS::MemoryPool *pool, unsigned nOperands)
        : m_irNode(irNode)
        , m_operands(pool, nOperands)
    {}

    ~MIInstr() = default;

public:
    static MIInstr *create(QQmlJS::MemoryPool *pool, Node *irNode, unsigned nOperands);

    MIBlock *parent() const
    { return m_parent; }

    MIBlock *getParent() const // for ilist_node_with_parent
    { return parent(); }

    void setParent(MIBlock *parent)
    { m_parent = parent; }

    Node *irNode() const
    { return m_irNode; }

    Operation::Kind opcode() const
    { return m_irNode->opcode(); }

    int position() const
    { return m_position; }

    inline void insertBefore(MIInstr *insertPos);
    inline void insertAfter(MIInstr *insertPos);
    inline MIInstrList<MIInstr>::iterator eraseFromParent();

    bool hasDestination() const
    { return m_destination.isValid(); }

    MIOperand destination() const
    { return m_destination; }

    void setDestination(const MIOperand &dest)
    { m_destination = dest; }

    const MIOperand &operand(unsigned index) const
    { return m_operands.at(index); }

    void setOperand(unsigned index, const MIOperand &op)
    { m_operands.at(index) = op; }

    MIOperand &operand(unsigned index)
    { return m_operands.at(index); }

    const MIOperand::List &operands() const
    { return m_operands; }

    MIOperand::List &operands()
    { return m_operands; }

private:
    friend MIFunction;
    void setPosition(int newPosition)
    { m_position = newPosition; }

private:
    MIBlock *m_parent = nullptr;
    Node *m_irNode = nullptr;
    int m_position = -1;
    MIOperand m_destination;
    MIOperand::List m_operands;
};

class MIBlock final
{
    Q_DISABLE_COPY_MOVE(MIBlock)

public:
    using Index = unsigned;
    enum : Index { InvalidIndex = std::numeric_limits<Index>::max() };

    using MIInstructionList = MIInstrList<MIInstr>;

    using InEdges = QVarLengthArray<MIBlock *, 4>;
    using OutEdges = QVarLengthArray<MIBlock *, 2>;

protected:
    friend MIFunction;
    explicit MIBlock(Index index)
        : m_instructions(this),
          m_index(index)
    {}

    void setIndex(Index newIndex)
    { m_index = newIndex; }

public:
    ~MIBlock() = default;

    const MIInstructionList &instructions() const
    { return m_instructions; }

    MIInstructionList &instructions()
    { return m_instructions; }

    static MIInstructionList MIBlock::*getSublistAccess(MIInstr * = nullptr)
    { return &MIBlock::m_instructions; }

    void addArgument(MIOperand &&arg)
    { m_arguments.push_back(arg); }

    const std::vector<MIOperand> &arguments() const
    { return m_arguments; }

    std::vector<MIOperand> &arguments()
    { return m_arguments; }

    void clearArguments()
    { m_arguments.resize(0); }

    const InEdges &inEdges() const
    { return m_inEdges; }

    void addInEdge(MIBlock *edge)
    { m_inEdges.append(edge); }

    const OutEdges &outEdges() const
    { return m_outEdges; }

    void addOutEdge(MIBlock *edge)
    { m_outEdges.append(edge); }

    Index index() const
    { return m_index; }

    MIBlock *findEdgeTo(Operation::Kind target) const;

    bool isDeoptBlock() const
    { return m_isDeoptBlock; }

    void markAsDeoptBlock()
    { m_isDeoptBlock = true; }

private:
    std::vector<MIOperand> m_arguments;
    MIInstructionList m_instructions;
    InEdges m_inEdges;
    OutEdges m_outEdges;
    Index m_index;
    bool m_isDeoptBlock = false;
};

class MIFunction final
{
    Q_DISABLE_COPY_MOVE(MIFunction)

public:
    static constexpr MIBlock::Index StartBlockIndex = 0;

public:
    MIFunction(Function *irFunction);
    ~MIFunction()
    { qDeleteAll(m_blocks); }

    Function *irFunction() const
    { return m_irFunction; }

    void setStartBlock(MIBlock *newStartBlock);
    void renumberBlocks();
    void renumberInstructions();

    void dump(const QString &description) const;

    size_t blockCount() const
    { return blocks().size(); }

    MIBlock *block(MIBlock::Index index) const
    { return m_blocks[index]; }

    const std::vector<MIBlock *> &blocks() const
    { return m_blocks; }

    MIBlock *addBlock()
    {
        auto *b = new MIBlock(unsigned(m_blocks.size()));
        m_blocks.push_back(b);
        return b;
    }

    void setBlockOrder(const std::vector<MIBlock *> &newSequence)
    { m_blocks = newSequence; }

    unsigned vregCount() const
    { return m_vregCount; }

    void setVregCount(unsigned vregCount)
    { m_vregCount = vregCount; }

    unsigned dwordSlotCount() const
    { return m_dwordSlotCount; }

    unsigned qwordSlotCount() const
    { return m_qwordSlotCount; }

    unsigned jsSlotCount() const
    { return m_jsSlotCount; }

    unsigned extraJSSlots() const;

    void setStackSlotCounts(unsigned dword, unsigned qword, unsigned js);

    void verifyCFG() const;

private:
    Function *m_irFunction = nullptr;
    std::vector<MIBlock *> m_blocks;
    unsigned m_vregCount = 0;
    unsigned m_dwordSlotCount = 0;
    unsigned m_qwordSlotCount = 0;
    unsigned m_jsSlotCount = 0;
};

Type MIOperand::nodeType(MIFunction *f) const
{
    return f->irFunction()->nodeInfo(irNode())->type();
}

inline uint qHash(const MIOperand &key, uint seed)
{
    uint h = ::qHash(key.kind(), seed);
    switch (key.kind()) {
    case MIOperand::VirtualRegister:
        h ^= key.virtualRegister();
        break;
    case MIOperand::BoolStackSlot: Q_FALLTHROUGH();
    case MIOperand::JSStackSlot:
        h ^= key.stackSlot();
        break;
    default:
        qFatal("%s: cannot hash %s", Q_FUNC_INFO, key.debugString().toUtf8().constData());
    }
    return h;
}

void MIInstr::insertBefore(MIInstr *insertPos)
{
    insertPos->parent()->instructions().insert(insertPos->getIterator(), this);
}

void MIInstr::insertAfter(MIInstr *insertPos)
{
    insertPos->parent()->instructions().insert(++insertPos->getIterator(), this);
}

MIInstrList<MIInstr>::iterator MIInstr::eraseFromParent()
{
    auto p = parent();
    setParent(nullptr);
    return p->instructions().erase(getIterator());
}

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4MI_P_H

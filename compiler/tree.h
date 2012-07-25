/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __TREE_H
#define __TREE_H

#include "compiler.h"
#include "../common/consts.h"
#include "../utils/FormatableString.h"
#include <vector>
#include <string>
#include <ostream>
#include <climits>



namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/

	//! Return the string corresponding to the binary operator
	std::wstring binaryOperatorToString(AsebaBinaryOperator op);
	
	//! Return the string corresponding to the unary operator
	std::wstring unaryOperatorToString(AsebaUnaryOperator op);
	
	//! An abstract node of syntax tree
	struct Node
	{
		//! A type a node can return
		enum ReturnType
		{
			TYPE_UNIT = 0,
			TYPE_BOOL,
			TYPE_INT
		};
		
		//! Constructor
		Node(const SourcePos& sourcePos) : sourcePos(sourcePos) { }		
		virtual ~Node();
		virtual Node* shallowCopy() = 0;
		virtual Node* deepCopy();
		
		//! Second pass to expand "abstract" nodes into more concrete ones
		virtual Node* treeExpand(std::wostream* dump, unsigned int index = 0);
		//! Typecheck this node, throw an exception if there is any type violation
		virtual ReturnType typeCheck() const;
		//! Optimize this node, return the optimized node
		virtual Node* optimize(std::wostream* dump) = 0;
		//! Return the stack depth requirement for this node and its children
		virtual unsigned getStackDepth() const;
		//! Generate bytecode
		virtual void emit(PreLinkBytecode& bytecodes) const = 0;
		
		//! Return a string representation of this node
		virtual std::wstring toWString() const = 0;
		//! Return a string representation of the name of this node
		virtual std::wstring toNodeName() const = 0;
		//! Dump this node and the rest of the tree
		virtual void dump(std::wostream& dest, unsigned& indent) const;

		//! Return the name of a type
		std::wstring typeName(const Node::ReturnType& type) const;
		//! Check for a specific type, throw an exception otherwise
		void expectType(const Node::ReturnType& expected, const Node::ReturnType& type) const;

		enum
		{
			E_NOVAL = 0
		};
		virtual unsigned getMemoryAddr() const;
		virtual unsigned getMemorySize() const;
		virtual void releaseChildren();

		//! Vector for children of a node
		typedef std::vector<Node *> NodesVector;
		NodesVector children; //!< children of this node
		SourcePos sourcePos; //!< position is source
	};
	
	//! Node for L"block", i.e. a vector of statements
	struct BlockNode : Node
	{
		//! Constructor
		BlockNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual BlockNode* shallowCopy() { return new BlockNode(*this); }

		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const { return L"Block"; }
		virtual std::wstring toNodeName() const { return L"block"; }
	};
	
	//! Node for L"program", i.e. a block node with some special behaviour later on
	struct ProgramNode: BlockNode
	{
		//! Constructor
		ProgramNode(const SourcePos& sourcePos) : BlockNode(sourcePos) { }
		virtual ProgramNode* shallowCopy() { return new ProgramNode(*this); }

		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const { return L"ProgramBlock"; }
		virtual std::wstring toNodeName() const { return L"program block"; }
	};
	
	//! Node for assignation.
	//! children[0] is store code
	//! children[1] expression to store
	struct AssignmentNode : Node
	{
		//! Constructor
		AssignmentNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual AssignmentNode* shallowCopy() { return new AssignmentNode(*this); }

		virtual Node* treeExpand(std::wostream* dump, unsigned int index);
		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const { return L"Assign"; }
		virtual std::wstring toNodeName() const { return L"assignment"; }
	};
	
	//! Node for L"if" and L"when".
	//! children[0] is expression
	//! children[1] is true block
	//! children[2] is false block
	struct IfWhenNode : Node
	{
		bool edgeSensitive; //!< if true, true block is triggered only if previous comparison was false ("when" block). If false, true block is triggered every time comparison is true
		unsigned endLine; //!< line of end keyword
		
		//! Constructor
		IfWhenNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual IfWhenNode* shallowCopy() { return new IfWhenNode(*this); }

		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"if/when"; }
	};
	
	//! Node for L"if" and L"when" with operator folded inside.
	//! children[0] is left part of expression
	//! children[1] is right part of expression
	//! children[2] is true block
	//! children[3] is false block
	struct FoldedIfWhenNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		bool edgeSensitive; //!< if true, true block is triggered only if previous comparison was false ("when" block). If false, true block is triggered every time comparison is true
		unsigned endLine; //!< line of end keyword
		
		//! Constructor
		FoldedIfWhenNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual FoldedIfWhenNode* shallowCopy() { return new FoldedIfWhenNode(*this); }

		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"folded if/when"; }
	};
	
	//! Node for L"while".
	//! children[0] is expression
	//! children[1] is block
	struct WhileNode : Node
	{
		//! Constructor
		WhileNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual WhileNode* shallowCopy() { return new WhileNode(*this); }

		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"while"; }
	};
	
	//! Node for L"while" with operator folded inside.
	//! children[0] is left part of expression
	//! children[1] is right part of expression
	//! children[2] is block
	struct FoldedWhileNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		
		//! Constructor
		FoldedWhileNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual FoldedWhileNode* shallowCopy() { return new FoldedWhileNode(*this); }

		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"folded while"; }
	};
	
	//! Node for L"onevent" 
	//! no children
	struct EventDeclNode : Node
	{
		unsigned eventId; //!< the event id associated with this context
		
		EventDeclNode(const SourcePos& sourcePos, unsigned eventId = 0);
		virtual EventDeclNode* shallowCopy() { return new EventDeclNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"event declaration"; }
	};
	
	//! Node for L"emit".
	//! may have children for pushing constants somewhere
	struct EmitNode : Node
	{
		unsigned eventId; //!< id of event to emit
		unsigned arrayAddr; //!< address of the first element of the array to send
		unsigned arraySize; //!< size of the array to send. 0 if event has no argument
		
		//! Constructor
		EmitNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual EmitNode* shallowCopy() { return new EmitNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"emit"; }
	};
	
	//! Node for L"sub"
	//! no children
	struct SubDeclNode : Node
	{
		unsigned subroutineId; //!< the associated subroutine
		
		SubDeclNode(const SourcePos& sourcePos, unsigned subroutineId);
		virtual SubDeclNode* shallowCopy() { return new SubDeclNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"subroutine declaration"; }
	};
	
	//! Node for L"callsub"
	//! no children
	struct CallSubNode : Node
	{
		unsigned subroutineId; //!< the subroutine to call
		
		CallSubNode(const SourcePos& sourcePos, unsigned subroutineId);
		virtual CallSubNode* shallowCopy() { return new CallSubNode(*this); }

		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"subroutine call"; }
	};
	
	//! Node for binary arithmetic.
	//! children[0] is left expression
	//! children[1] is right expression
	struct BinaryArithmeticNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		
		BinaryArithmeticNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		BinaryArithmeticNode(const SourcePos& sourcePos, AsebaBinaryOperator op, Node *left, Node *right);
		virtual BinaryArithmeticNode* shallowCopy() { return new BinaryArithmeticNode(*this); }

		void deMorganNotRemoval();
		
		virtual Node* treeExpand(std::wostream* dump, unsigned int index = 0);
		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"binary function"; }
		
		static BinaryArithmeticNode *fromComparison(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromShiftExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromAddExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromMultExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromBinaryExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
	};
	
	//! Node for unary arithmetic
	//! children[0] is the expression to negate
	struct UnaryArithmeticNode : Node
	{
		AsebaUnaryOperator op; //!< operator
		
		//! Constructor
		UnaryArithmeticNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		UnaryArithmeticNode(const SourcePos& sourcePos, AsebaUnaryOperator op, Node *child);
		virtual UnaryArithmeticNode* shallowCopy() { return new UnaryArithmeticNode(*this); }

		virtual Node* treeExpand(std::wostream* dump, unsigned int index = 0);
		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"unary function"; }
	};
	
	//! Node for pushing immediate value on stack.
	//! Might generate either Small Immediate or Large Immediate bytecode
	//! depending on the range of value
	//! no children
	struct ImmediateNode : Node
	{
		int value; //!< value to push on stack
		
		//! Constructor
		ImmediateNode(const SourcePos& sourcePos, int value) : Node(sourcePos), value(value) { }
		virtual ImmediateNode* shallowCopy() { return new ImmediateNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_INT; }
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"constant"; }
		virtual unsigned getMemorySize() const { return 1; }
	};
	
	//! Node for storing a variable from stack.
	//! no children
	struct StoreNode : Node
	{
		unsigned varAddr; //!< address of variable to store to
		
		//! Constructor
		StoreNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		virtual StoreNode* shallowCopy() { return new StoreNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"variable access (write)"; }
		virtual unsigned getMemoryAddr() const { return varAddr; }
		virtual unsigned getMemorySize() const { return 1; }
	};
	
	//! Node for loading a variable on stack.
	//! no children
	struct LoadNode : Node
	{
		unsigned varAddr; //!< address of variable to load from

		//! Constructor
		LoadNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		LoadNode(const StoreNode* store) : Node(store->sourcePos), varAddr(store->varAddr) {}
		virtual LoadNode* shallowCopy() { return new LoadNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_INT; }
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"variable access (read)"; }
		virtual unsigned getMemoryAddr() const { return varAddr; }
		virtual unsigned getMemorySize() const { return 1; }
	};

	//! Node for writing to an array. Value to store is supposed to be on the stack already
	//! children[0] is the index in the array
	struct ArrayWriteNode : Node
	{
		unsigned arrayAddr; //!< address of the first element of the array
		unsigned arraySize; //!< size of the array, might be used to assert compile-time access checks
		std::wstring arrayName; //!< name of the array (for debug)
		
		ArrayWriteNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName);
		virtual ArrayWriteNode* shallowCopy() { return new ArrayWriteNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"array access (write)"; }
		virtual unsigned getMemoryAddr() const { return arrayAddr; }
		virtual unsigned getMemorySize() const { return 1; }
	};
	
	//! Node for reading from an array.
	//! children[0] is the index in the array
	struct ArrayReadNode : Node
	{
		unsigned arrayAddr; //!< address of the first element of the array
		unsigned arraySize; //!< size of the array, might be used to assert compile-time access checks
		std::wstring arrayName; //!< name of the array (for debug)

		ArrayReadNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName);
		ArrayReadNode(const ArrayWriteNode* write) :
			Node(write->sourcePos), arrayAddr(write->arrayAddr), arraySize(write->arraySize), arrayName(write->arrayName)
		{
			ImmediateNode* index;
			// accept only immediates for array index
			// TODO fix this? should handle the case:
			// array[i++]+=2, which will expand to array[i++] = array[i++] + 2
			// which is not what the user meant
			if (write->children.size() > 0 && (index = dynamic_cast<ImmediateNode*> (write->children[0])) != 0)
				children.push_back(new ImmediateNode(*index));
			else
				throw Error(sourcePos, WFormatableString(L"Such operation is not permitted with non-immediate index"));
		}
		virtual ArrayReadNode* shallowCopy() { return new ArrayReadNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_INT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"array access (read)"; }
		virtual unsigned getMemoryAddr() const { return arrayAddr; }
		virtual unsigned getMemorySize() const { return 1; }
	};

	//! Node for calling a native function
	//! may have children for pushing constants somewhere
	struct CallNode : Node
	{
		unsigned funcId; //!< identifier of the function to be called
		std::vector<unsigned> argumentsAddr; //!< address of all arguments
		
		CallNode(const SourcePos& sourcePos, unsigned funcId);
		virtual CallNode* shallowCopy() { return new CallNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"native function call"; }
	};
	
	//! Node for returning from an event or subroutine
	//! has no children, just a jump of 0 offset that will be resolved at link time
	struct ReturnNode : Node
	{
		ReturnNode(const SourcePos& sourcePos) : Node(sourcePos) {}
		virtual ReturnNode* shallowCopy() { return new ReturnNode(*this); }

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump) { return this; }
		virtual unsigned getStackDepth() const { return 0; }
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const { return L"Return"; }
		virtual std::wstring toNodeName() const { return L"return"; } 
	};

	struct VectorNode : Node
	{
		VectorNode(const SourcePos& sourcePos) : Node(sourcePos) {}

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump) = 0;
		virtual void emit(PreLinkBytecode& bytecodes) const = 0;
		virtual std::wstring toWString() const = 0;
		virtual std::wstring toNodeName() const = 0;
	};

	//! Node for assembling values into an array
	//! values[x] is the x-th value to be assembled
	struct StaticVectorNode : VectorNode
	{
		std::vector<int> values;

		//! Constructor
		StaticVectorNode(const SourcePos& sourcePos) : VectorNode(sourcePos) {}
		StaticVectorNode(const SourcePos& sourcePos, int value) : VectorNode(sourcePos) { values.push_back(value); }
		virtual StaticVectorNode* shallowCopy() { return new StaticVectorNode(*this); }

		virtual Node* treeExpand(std::wostream* dump, unsigned int index = 0);
		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"array constructor"; }
		virtual void dump(std::wostream& dest, unsigned& indent) const;

		virtual unsigned getMemorySize() const { return values.size(); }

		virtual int getLonelyImmediate() const;
		virtual int getValue(unsigned index) const;
		virtual void addValue(int value) { values.push_back(value); }
	};

	//! Node for accessing a memory as a vector, in read or write operations
	//! This is an abstraction, and should be replaced by the correct operation(s) at a later stage
	//! children[0] is optional index, like 'i' in foo[i]
	//! If children[0] is a StaticVector of two elements (int), it will be foo[x:y]
	struct MemoryVectorNode : VectorNode
	{
		unsigned arrayAddr; //!< address of the first element of the array
		unsigned arraySize; //!< size of the array, might be used to assert compile-time access checks
		std::wstring arrayName; //!< name of the array (for debug)
		bool write;

		MemoryVectorNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName);
		virtual MemoryVectorNode* shallowCopy() { return new MemoryVectorNode(*this); }

		virtual Node* treeExpand(std::wostream* dump, unsigned int index = 0);
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"vector access"; }
		virtual unsigned getMemoryAddr() const;
		virtual unsigned getMemorySize() const;

		virtual void setWrite(bool write) { this->write = write; }
	};

	/*@}*/
	
}; // Aseba

#endif

#include <unordered_map>

#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

class AIPass : public FunctionPass {
public:
    static char ID;
    AIPass() : FunctionPass(ID) {}

    bool runOnFunction(Function& F) override {
	for (auto& BB : F) {
	    for (auto& I : BB) {
		if (!I.getType()->isIntegerTy())
		    continue;

		if (!I.isBinaryOp())
		    continue;

		IntegerType* intType = dyn_cast<IntegerType>(I.getType());
		assert(intType);

		const unsigned bitWidth = intType->getBitWidth();
		Value* v1 = I.getOperand(0);
		Value* v2 = I.getOperand(1);
		const ConstantRange& r1 = getRange(v1, bitWidth);
		const ConstantRange& r2 = getRange(v2, bitWidth);
		ConstantRange result(bitWidth, true);

		switch(I.getOpcode()) {
		case Instruction::Add:
		    result = r1.add(r2);
		    break;
		case Instruction::Sub:
		    result = r1.sub(r2);
		    break;
		default:
		    break;
		}

		_map.emplace(dyn_cast<Value>(&I), result);
		outs() << "Instruction: " << I
		       << " , CR: " << result << "\n";
	    }
	}
    }
private:
    std::unordered_map<Value*, ConstantRange> _map;

private:
    // Returns the already calculated range for val
    // If not exists, create a new range of width = bitWidth
    const ConstantRange& getRange(Value* val, const unsigned bitWidth) {
	if (_map.find(val) == _map.end()) {
	    if (auto constInt = dyn_cast<ConstantInt>(val))
		_map.emplace(val, constInt->getValue());
	    else
		_map.emplace(val, ConstantRange(bitWidth, true)); // full-set
	}

	return _map.at(val);
    }
};

char AIPass::ID = 42;
static RegisterPass<AIPass> X("ai-pass", "Simple Abstract Interpreter",
			      false,
			      false);

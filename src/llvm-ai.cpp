#include <unordered_map>
#include <queue>

#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

enum class LatticeVal : char {
    // don't need any Top because ConstantRange will be limited
    // by its bitwidth.
    ConstantRange,
    Bottom,
};

class AIPass : public FunctionPass {
public:
    static char ID;
    AIPass() : FunctionPass(ID) {}

    bool runOnFunction(Function& F) override {
	// create a worklist
	std::queue<Value*> worklist = init(F);
	while (!worklist.empty()) {
	    Value* val = worklist.front(); worklist.pop();

	    if (isa<Instruction>(val))
		outs() << *cast<Instruction>(val) << "\n";

	    std::pair<LatticeVal, ConstantRange*> oldres = getCurrentValue(val);
	    std::pair<LatticeVal, ConstantRange*> newres = processValue(val);
	    if (oldres.second != newres.second) {
		// delete any existing ConstantRanges, if any
		if (_mapRanges.find(val) != _mapRanges.find(val)) {
		    if (_mapRanges.at(val) != nullptr) {
			delete _mapRanges.at(val);
		    }
		}

		_mapLatticeVals[val] = newres.first;
		_mapRanges[val] = newres.second;

		// TODO: put all dependents in the worlist,
		// only if they are not already in
		for (auto user : val->users()) {
		    worklist.push(cast<Value>(user));
		}
	    }
	}

	dumpAnalysis();

	return false;
    }
private:
    // contains some value of the lattice, LatticeVal
    std::unordered_map<Value*, LatticeVal> _mapLatticeVals;

    // If _mapLatticeVals of some Value* is LatticeVal::ConstantRange, the
    // actual constant range is stored here
    std::unordered_map<Value*, ConstantRange*> _mapRanges;

private:
    std::pair<LatticeVal, ConstantRange*> processValue(Value* val) {
	std::pair<LatticeVal, ConstantRange*> result;

	// we know nothing about non-integers!!!
	if (!val->getType()->isIntegerTy())
	    return {LatticeVal::Bottom, nullptr};

	unsigned bitWidth = val->getType()->getIntegerBitWidth();
	outs() << "processValue: bitWidth: " << bitWidth << "\n";

	if (auto *fnarg = dyn_cast<Argument>(val)) {
	    outs() << "processValue: is a fn arg: " << "\n";
	    outs() << fnarg->getParent() << "\n";
	    return {LatticeVal::ConstantRange,
		    new ConstantRange(bitWidth, true)};
	}
	else if (auto* cint = dyn_cast<ConstantInt>(val)) {
	    outs() << "processValue: is a const inst: " << "\n";
	    return {LatticeVal::ConstantRange,
		    new ConstantRange(cint->getValue())};
	}
	else if (auto* callinst = dyn_cast<CallInst>(val)) {
	    outs() << "processValue: is a call inst: " << "\n";
	    return {LatticeVal::ConstantRange,
		    new ConstantRange(bitWidth, true)};
	}
	else if (auto* inst = dyn_cast<Instruction>(val)) {
	    outs() << "processValue: xfunc\n";
	    return xfunc(val);
	}

	outs() << "nothing processed\n";
    }

    // execute appropriate transfer function on val
    std::pair<LatticeVal, ConstantRange*> xfunc(Value* val) {
	Instruction* inst = cast<Instruction>(val);
	assert(inst);

	std::pair<LatticeVal, ConstantRange*> defaultpair =
	    {LatticeVal::Bottom,
	     nullptr};

	if (!val->getType()->isIntegerTy())
	    return defaultpair;

	// TODO: Extend to non-binary ops transfer functions too
	if (!inst->isBinaryOp())
	    return defaultpair;

	const unsigned bitWidth = val->getType()->getIntegerBitWidth();
	Value* v1 = inst->getOperand(0);
	Value* v2 = inst->getOperand(1);
	auto res1 = getCurrentValue(v1);
	auto res2 = getCurrentValue(v2);
	if (res1.first == LatticeVal::Bottom ||
	    res2.first == LatticeVal::Bottom)
	    return defaultpair;

	const ConstantRange& r1 = *res1.second;
	const ConstantRange& r2 = *res2.second;
	ConstantRange result(bitWidth, true);

	switch(inst->getOpcode()) {
	case Instruction::Add:
	    result = r1.add(r2);
	    return {LatticeVal::ConstantRange,
		    new ConstantRange(result)};
	    break;
	case Instruction::Sub:
	    result = r1.sub(r2);
	    return {LatticeVal::ConstantRange,
		    new ConstantRange(result)};
	    break;
	default:
	    return defaultpair;
	}
    }

    std::pair<LatticeVal, ConstantRange*> getCurrentValue(Value* val) {
	if (auto* cint = dyn_cast<ConstantInt>(val)) {
	    outs() << "getCurrentValue: is a const inst: " << "\n";
	    _mapLatticeVals[val] = LatticeVal::ConstantRange;
	    _mapRanges[val] = new ConstantRange(cint->getValue());
	}

	assert(_mapLatticeVals.find(val) != _mapLatticeVals.end());
	assert(_mapRanges.find(val) != _mapRanges.end());

	return {_mapLatticeVals.at(val), _mapRanges.at(val)};
    }

    // Initialize all values -- assign appropriate lattice elements, etc.
    std::queue<Value*> init(Function& F) {
        // if you don't know about something, init to full range
	// otherwise, it's a constant
	auto fnIterator = F.arg_begin();
	while (fnIterator != F.arg_end()) {
	    Value* arg = fnIterator;

	    std::pair<LatticeVal, ConstantRange*> res = processValue(arg);
	    _mapLatticeVals[arg] = res.first;
	    _mapRanges[arg] = res.second;

	    fnIterator++;
	}

	std::queue<Value*> worklist;
	outs() << "initing\n";
	for (auto& BB : F) {
	    for (auto& I : BB) {
		outs() << I << "\n";
		Value* val = cast<Value>(&I);

		std::pair<LatticeVal, ConstantRange*> res = processValue(val);
		_mapLatticeVals[val] = res.first;

		// delete any existing constant ranges
		if (_mapRanges.find(val) != _mapRanges.end()) {
		    if (_mapRanges.at(val) != nullptr)
			delete _mapRanges.at(val);
		}
		_mapRanges[val] = res.second;

		worklist.push(val);
	    }
	}

	return worklist;
    }

    void dumpAnalysis() {
	outs() << "Dump Analysis Results\n";
	outs() << "========================\n";

	for (auto& val : _mapRanges) {
	    if (!isa<Instruction>(val.first))
		continue;
	    outs() << *cast<Instruction>(val.first) << "\n";
	    if (!val.second) {
		outs() << "No result\n";
		continue;
	    }
	    else if (auto sele = val.second->getSingleElement()) {
		outs() << "\tResult: "
		       << *sele << "\n";
	    }
	    else {
		outs() << "Result: "
		       << *val.second << "\n";
	    }
	}
    }
};

char AIPass::ID = 42;
static RegisterPass<AIPass> X("ai-pass", "Simple Abstract Interpreter",
			      false,
			      false);

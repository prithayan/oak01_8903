//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/VectorUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/Loads.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopPassManager.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PredIteratorCache.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include <algorithm>


#include <vector>
#include <deque>
#include <set>
#include <stack>
#include <map>
#include <algorithm>
#include <ostream>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace llvm;

#define DEBUG_TYPE "hello"

//STATISTIC(HelloCounter, "Counts number of functions greeted");

namespace {
    class  defuseGraph {
        struct  node {
            Instruction *instr;
            std::vector<node *> parent;
            std::vector<node *> children;
            node(Instruction *i) {
                instr = i;
                instr_node_map[instr] = this;
            }
        };
        static std::map<Instruction*,node*> instr_node_map;
        std::set<node*> forest_roots;
        std::set<node*> forest_leaves;
        std::ofstream flat_defuse_print_file;
        std::set<node* > leaf_root_dfs_visited_set;
        std::string func_name_str;

        void DFS_traversal( ) {
            std::set<node*>::iterator it ,end;
            for (it=forest_roots.begin(), end=forest_roots.end();it!=end;it++ ){
                errs()<<"\n Root::"<<*(*it)->instr;
                std::stack<node*> dfs_stack;
                std::set<node*> visited_nodes_set;
                dfs_stack.push(*it);
                while (!dfs_stack.empty() ) {
                    node* cur_node = dfs_stack.top();
                    Instruction *cur_instr = cur_node->instr;
                    errs()<<"\n Popped::"<<cur_instr->getOpcodeName()<<"__"<<*cur_instr->getType();
                    dfs_stack.pop();
                    if (visited_nodes_set.find(cur_node) == visited_nodes_set.end() ) {//if not visited 
                        visited_nodes_set.insert(cur_node );
                        std::vector<node*>::iterator it,end;
                        for (it=cur_node->children.begin(),end=cur_node->children.end();it!=end;it++ ) {
                            if (visited_nodes_set.find(*it) == visited_nodes_set.end() ) {
                                dfs_stack.push(*it );
                            }
                        }
                    }
                }
            }
            //
            //	DFS(G,v)   ( v is the vertex where the search starts )
            //		 Stack S := {};   ( start with an empty stack )
            //     for each vertex u, set visited[u] := false;
            //     push S, v;
            //     while (S is not empty) do
            //        u := pop S;
            //        if (not visited[u]) then
            //           visited[u] := true;
            //           for each unvisited neighbour w of u
            //              push S, w;
            //        end if
            //     end while
            //  END DFS()
        }

        void leaf_root_DFS(node *cur_node,std::string  leaf_root_path_str ) {
            //errs()<<*cur_node->instr;
            static bool last_instr_branch = false  ;
            if (!( isa<BranchInst>( cur_node->instr) && last_instr_branch)){
                std::string str;
                std::string instr_opcode_str(cur_node->instr->getOpcodeName() );
                llvm::raw_string_ostream rso(str);
                cur_node->instr->getType()->print(rso);

                if ( CallInst *calledI = dyn_cast<CallInst>(cur_node->instr)) {
                    if ( Function *F = calledI->getCalledFunction()) {
                        rso<<F->getName();
                    }
                }
                rso.flush();
                //errs()<<"\n type::"<<*cur_node->instr->getType()<<"\n and str::"<<str;


                //std::stringstream cur_instr_str;
                //cur_instr_str << (*- <<std::endl;
                //leaf_root_path_str.append(" ");
                leaf_root_path_str.append(instr_opcode_str );
                leaf_root_path_str.append("_");
                leaf_root_path_str.append(str );
                leaf_root_path_str.append(" ");
            }
            if ( !last_instr_branch && isa<BranchInst>( cur_node->instr)) last_instr_branch = true;
            if (leaf_root_dfs_visited_set.find(cur_node) != leaf_root_dfs_visited_set.end())
                return;
            leaf_root_dfs_visited_set.insert(cur_node);
            if (cur_node->parent.size() == 0 ) {
                print_defuse_string(leaf_root_path_str );
            }
            for (int i=0,end=cur_node->parent.size();i<end;i++) {

                leaf_root_DFS(cur_node->parent[i], leaf_root_path_str);
            }
            return;
        }

        void print_defuse_string(std::string leaf_root_path_str) {
            std::string line_str(leaf_root_path_str );
            const char* check_num_spaces = line_str.c_str();
            int count=0;
            for (int i=0; check_num_spaces[i] != 0 ; i++) {
                if (check_num_spaces[i] == ' ' ) count++;
                if (count == 2) break;
            }
            if (count == 1) return;
            std::replace(line_str.begin(), line_str.end(), ' ', '_');
            flat_defuse_print_file<< "\n"<<func_name_str;
            //flat_defuse_print_file<< "\n"<<line_str;
            flat_defuse_print_file<<"  "<<leaf_root_path_str;
            flat_defuse_print_file<< "  "<<func_name_str;
        }

        void leaf_root_path() {

            for (std::set<node*>::iterator it=forest_leaves.begin(), end=forest_leaves.end();it!=end;it++ ){

                leaf_root_dfs_visited_set.clear();
                leaf_root_DFS(*it, "" );
            }
        }
public:
        defuseGraph() {
            flat_defuse_print_file.open("defuse_dump_file.txt", std::ofstream::out | std::ofstream::app);
        }
        void add_def_use(Instruction *def, Instruction *use ) {
            node* def_node;
            node *use_node ;
            if (instr_node_map.find(def) == instr_node_map.end() ) { //new instr add everywhere
                def_node = new node(def );
                forest_roots.insert(def_node );
            }else {//old instr, simply update everywhere
                def_node = instr_node_map[def];
                forest_leaves.erase(def_node);
            }
            if (instr_node_map.find(use) == instr_node_map.end() ) { //new instr add everywhere
                use_node = new node(use );
                forest_leaves.insert(use_node);
            }else {//old instr, simply update everywhere
                use_node = instr_node_map[use];
                forest_roots.erase(use_node );
            }
            def_node->children.push_back(use_node );
            use_node->parent.push_back(def_node );
        }
        void print(std::string label_str) {
            flat_defuse_print_file <<"\n%"<< label_str ;
        }
        void print() {
            leaf_root_path() ;
            //DFS_traversal();
        }
        void set_func_name_str(std::string s) {func_name_str=s;}
    };
 std::map<Instruction*,defuseGraph::node*>   defuseGraph::instr_node_map;
    // Insert Block Stride Computation Instruction 
    struct ComputeBlockStride : public FunctionPass {
        static char ID; // Pass identification, replacement for typeid
        Module *thisMod;
        std::set<Value* > visit_done_set;
        std::string filename_str;
        ComputeBlockStride() : FunctionPass(ID) { 
        }
        void set_fileName(Function &Func ) {

            for (inst_iterator instructionIterator = inst_begin(Func), E = inst_end(Func); instructionIterator
                    != E; ++instructionIterator) {
                Instruction *instr = &*(instructionIterator);
                DILocation *loc = instr->getDebugLoc();  
                if (loc) {
                    errs()<<"\n Filename::"<<loc->getFilename();
                    filename_str = loc->getFilename();  
                    return;
                }
            }
        }

        bool runOnFunction(Function &Func) override {
    defuseGraph DU_defuseGraph;
            errs() << "Running Pass on Function::";
            errs().write_escaped(Func.getName()) << '\n';
            visit_done_set.clear();
            // func is a pointer to a Function instance
            //set_fileName(Func);

            // for (Function::iterator basicBlockIterator = Func.begin(), basicBlockEnd = Func.end(); basicBlockIterator != basicBlockEnd; ++basicBlockIterator) 
            //errs() << "Basic block (name=" << basicBlockIterator->getName() << ") has "<< basicBlockIterator->size() << " instructions.\n";
            //for (BasicBlock::iterator instructionIterator = basicBlockIterator->begin(), instructionEnd =
            //       basicBlockIterator->end(); instructionIterator != instructionEnd;++instructionIterator) 
            for (inst_iterator instructionIterator = inst_begin(Func), E = inst_end(Func); instructionIterator
                    != E; ++instructionIterator) {
                //errs() << *I << "\n";
                //errs() << "\n Def:: "<<*instructionIterator << "\n";
                Instruction *instr = &*(instructionIterator);
                if (filename_str.empty() ){
                    DILocation *loc = instr->getDebugLoc();  
                    if (loc) {
                        errs()<<"\n Filename::"<<loc->getFilename();
                        filename_str = loc->getFilename();  
                    }
                }
                for (Use &U : instr->uses()) {
                    Instruction *User = cast<Instruction>(U.getUser());
                    //errs()<<"\n User::"<<*User;
                    DU_defuseGraph.add_def_use(instr,User );
                }
                if (BranchInst *bi = dyn_cast<BranchInst>(instr ) ) {

                    for (int i=0, e=bi->getNumSuccessors();i<e;i++ ){
                        BasicBlock* branchTbb = bi->getSuccessor(i);
                        for (BasicBlock::iterator branch_targIterator = branchTbb->begin(), branchTbbEnd =
                                branchTbb->end(); branch_targIterator != branchTbbEnd;++branch_targIterator) {
                            Instruction *target_instr = dyn_cast<Instruction>(branch_targIterator);
                            DU_defuseGraph.add_def_use(instr,target_instr );
                        }
                    }
                }
            }

            std::string funcName_file(Func.getName());
            funcName_file.append("&&");
            funcName_file.append(filename_str );
            DU_defuseGraph.set_func_name_str(funcName_file );
            DU_defuseGraph.print(funcName_file );
            DU_defuseGraph.print();
            return false;
        }

        // We don't modify the program, so we preserve all analyses.
        void getAnalysisUsage(AnalysisUsage &AU) const override {
            //AU.setPreservesCFG();
            AU.setPreservesAll();
            //AU.addRequired<TargetLibraryInfoWrapperPass>();
            //AU.addRequired<DominatorTreeWrapperPass>();
            //AU.addRequired<CallGraphWrapperPass>();
            //AU.addRequired<LoopInfoWrapperPass>();
            //getLoopAnalysisUsage(AU);
        }
    };
}

char ComputeBlockStride::ID=0;
static RegisterPass<ComputeBlockStride>
MY("compute_block_stride", "Compute Block Stride CUDA");

static void registerComputeStridePass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
/*PassRegistry &Registry = *PassRegistry::getPassRegistry();
   initializeAnalysis(Registry);
initializeCore(Registry);
                initializeScalarOpts(Registry);
                initializeIPO(Registry);
                initializeAnalysis(Registry);
                initializeIPA(Registry);
                initializeTransformUtils(Registry);
                initializeInstCombine(Registry);
                initializeInstrumentation(Registry);
                initializeTarget(Registry);*/
  PM.add(new ComputeBlockStride());
  errs()<<"\n Registered my pass--------";
}
//static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_LoopOptimizerEnd,registerComputeStridePass);
//static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_ModuleOptimizerEarly,registerComputeStridePass);
//static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_ScalarOptimizerLate,registerComputeStridePass);
static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible, registerComputeStridePass);
//static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_OptimizerLast,registerComputeStridePass);
//namespace {
//  // Hello - The first implementation, without getAnalysisUsage.
//  struct Hello : public FunctionPass {
//    static char ID; // Pass identification, replacement for typeid
//    Hello() : FunctionPass(ID) {}
//
//    bool runOnFunction(Function &F) override {
//      ++HelloCounter;
//      errs() << "Hello: func1 ";
//      errs().write_escaped(F.getName()) << '\n';
//      return false;
//    }
//  };
//}
//
//char Hello::ID = 0;
//static RegisterPass<Hello> X("hello", "Hello World Pass");
//
//namespace {
//  // Hello2 - The second implementation with getAnalysisUsage implemented.
//  struct Hello2 : public FunctionPass {
//    static char ID; // Pass identification, replacement for typeid
//    Hello2() : FunctionPass(ID) {}
//
//    bool runOnFunction(Function &F) override {
//      ++HelloCounter;
//      errs() << "Hello function2 by prithayan: ";
//      errs().write_escaped(F.getName()) << '\n';
//      return false;
//    }
//
//    // We don't modify the program, so we preserve all analyses.
//    void getAnalysisUsage(AnalysisUsage &AU) const override {
//      AU.setPreservesAll();
//    }
//  };
//}
//
//char Hello2::ID = 0;
//static RegisterPass<Hello2>
//Y("hello2", "Hello World Pass (with getAnalysisUsage implemented)");
//
//
//
//
//namespace {
//  // Loop_info : this pass will analyze the loop index and print instructions that use the loop index
//
//  struct mem_stride_info{
//      //memory and the UB and the index stride
//      std::set<unsigned int> mem_argNums;
//      const SCEV *upperBound;
//      const SCEV *stepRecr;
//  };
//  std::map<Function*, mem_stride_info*> functionStrideMap;
//  struct MyLoopInfo : public LoopPass {
//  static char ID; // Pass identification, replacement for typeid
//  Module *thisModule;
//  MyLoopInfo () : LoopPass(ID) {
//      thisModule = nullptr;
//
//  }
//
//  bool runOnLoop(Loop *L, LPPassManager &LPM) override {
//      //const Function *   getParent () const
//      auto *SE = getAnalysisIfAvailable<ScalarEvolutionWrapperPass>();
//      AnalyzeThisLoop(L,
//              &getAnalysis<AAResultsWrapperPass>().getAAResults(),
//              &getAnalysis<LoopInfoWrapperPass>().getLoopInfo(),
//              &getAnalysis<DominatorTreeWrapperPass>().getDomTree(),
//              &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(),
//              SE ? &SE->getSE() : nullptr, false);
//      std::map< Function*, mem_stride_info*>::iterator it;
//      for (it = functionStrideMap.begin() ; it!= functionStrideMap.end(); it++ ){
//          errs()<<"\n Function ::"<<(it->first)->getName();
//          errs()<<"\n upper bound:"<<*(it->second->upperBound);
//      }
//      //  Function *F = it->first;
//      //  if (F == nullptr) continue;
//      //  if (!( F->begin())) continue;
//      //  inst_iterator I    = inst_begin(F);
//      //  Instruction *instr = &*I; 
//      //  Module *M = instr->getModule();
//      //Module *M = functionStrideMap.begin()->first->getParent();
//      //insertStride_atMalloc(thisModule );
//      return true ;
//  }
//  static std::string getPrintfCodeFor(const Value *V) {
//      if (V == 0) return "";
//      if (V->getType()->isFloatTy())
//          return "%g";
//      else if (V->getType()->isIntegerTy())
//          return "%d";
//      assert(0 && "Illegal value to print out...");
//      return "";
//  }
//  static inline GlobalVariable *getStringRef(Module *M, const std::string &str) {
//      // Create a constant internal string reference...
//      Constant *Init =ConstantDataArray::getString(M->getContext(),str);
//      // Create the global variable and record it in the module
//      // The GV will be renamed to a unique name if needed.
//      GlobalVariable *GV = new GlobalVariable(Init->getType(), true,
//              GlobalValue::InternalLinkage, Init,
//              "trstr");
//      M->getGlobalList().push_back(GV);
//      return GV;
//  }
//
////  static void InsertPrintInst(Value *V,  Instruction *InsertBefore,
////                              std::string Message,
////                              Function *Printf) {
////      // Escape Message by replacing all % characters with %% chars.
////      BasicBlock *BB = InsertBefore->getParent();
////      std::string Tmp;
////      std::swap(Tmp, Message);
////      std::string::iterator I = std::find(Tmp.begin(), Tmp.end(), '%');
////      while (I != Tmp.end()) {
////          Message.append(Tmp.begin(), I);
////          Message += "%%";
////          ++I; // Make sure to erase the % as well...
////          Tmp.erase(Tmp.begin(), I);
////          I = std::find(Tmp.begin(), Tmp.end(), '%');
////      }
////      Message += Tmp;
////      Module *Mod = BB->getParent()->getParent();
////      // Turn the marker string into a global variable...
////      GlobalVariable *fmtVal = getStringRef(Mod,
////              Message+getPrintfCodeFor(V)+"\n");
////      // Turn the format string into an sbyte *
////      Constant *GEP=ConstantExpr::getGetElementPtr(fmtVal,
////              std::vector<Constant*>(2,Constant::getNullValue(Type::getInt64Ty(BB->getContext()))));
////      // // Insert a call to the hash function if this is a pointer value
////      // if (V && isa<PointerType>(V->getType()) && !DisablePtrHashing)
////      // {
////      //     const Type *SBP = PointerType::get(Type::SByteTy);
////      //     if (V->getType() != SBP)     // Cast pointer
////      //         to be sbyte*
////      //             V = new CastInst(V, SBP,
////      //                     "Hash_cast", InsertBefore);
////      //     std::vector<Value*> HashArgs(1,
////      //             V);
////      //     V = new
////      //         CallInst(HashPtrToSeqNum,
////      //                 HashArgs, "ptrSeqNum",
////      //                 InsertBefore);
////      // }
////      // Insert the first       print instruction to           print the string flag:
////      std::vector<Value*>           PrintArgs;
////      PrintArgs.push_back(GEP);
////      if (V)
////          PrintArgs.push_back(V);
////          CallInst *call_print = CallInst::Create(Printf, PrintArgs, "trace", InsertBefore);
////  }
////
////    Function *  getPrintfFunc(Module *M ) {
////      const Type *SBP = PointerType::get(Type::SByteTy);
////      const FunctionType *MTy = FunctionType::get(Type::IntTy, std::vector<const Type*>(1,SBP), true);
////      Function* printfFunc =M->getOrInsertFunction("printf", MTy);
////      return printfFunc;
////  }
//
//  void insertStride_atMalloc( Module *M){
//      Module::iterator it;
//      Module::iterator end = M->end();                                                                                                                                     
//	//Function* printfFunc =cast<Function>( M->getOrInsertFunction("printf"));
//	//Function* printfFunc =cast<Function>( M->getFunction("printf"));
//   // Function *printfFunc = getPrintfFunc(M );
//   // if (printfFunc!= nullptr)
//   // errs()<<"\n printf functin is :: "<<*printfFunc;
//
//      for (it = M->begin(); it != end; ++it) {
//          Function *F = &*it;
//          errs()<<"\n function::"<<*F;
//          errs()<<"=====================================";
//          for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I){
//              Instruction *instr = &*I;
//              errs() << *instr << "\n";
//              if (isa<CallInst>(instr)) {
//                  CallInst *calledI = dyn_cast<CallInst>(instr);
//                  IRBuilder<> builder(instr);
//                  if ( Function *F = calledI->getCalledFunction()) {
//                      std::map< Function*, mem_stride_info*>::iterator it = functionStrideMap.find(F) ;
//                      if (it != functionStrideMap.end() ) {
//                          errs()<<"\n got called func is ::"<<F->getName();
//                          const SCEV *UB = it->second->upperBound;
//                          Value *insertedinstr = createInstr_SCEV (UB, calledI, builder ) ;
//                          //InsertPrintInst(insertedinstr , instr, "upper bound is", printfFunc   );
//
//                         // if (printfFunc != nullptr)
//                         //     builder.CreateCall(printfFunc,  printf_arg_values );
//                          errs()<<"\n inserted the Instr::"<<*insertedinstr;
//                      }
//                  }
//              }
//          }
//      }
//      return ;
//  }
//
//              //  for (auto      &bb : iter->getBasicBlockList() ) {
//              //      //const BasicBlock *basicb = dyn_cast<BasicBlock>(bb);
//              //      for (auto i=bb.begin(); i!=bb.end();i++) {
//              //          Instruction *instr = dyn_cast<Instruction>(&i);
//              //          errs() <<"\n instruction is ::"<<*i;
//              //          //if (Instruction *instr = dyn_cast<Instruction*>( i)){
//              //          //errs() <<" \n instr::"<<instr;
//              //          //}
//
//              //      }
//              //  }
//
//  /// This transformation requires natural loop information & requires that
//  /// loop preheaders be inserted into the CFG...
//  ///
//  void getAnalysisUsage(AnalysisUsage &AU) const override {
//      AU.setPreservesCFG();
////    AU.setPreservesAll();
//    AU.addRequired<TargetLibraryInfoWrapperPass>();
//    getLoopAnalysisUsage(AU);
//  }
//
//
//private:
//
//
//
//// //define a extern function "printf"
//// static llvm::Function*	
//// printf_prototype(llvm::LLVMContext& ctx, llvm::Module *mod)
//// {
//// 		std::vector<llvm::Type*> printf_arg_types;
//// 		printf_arg_types.push_back(llvm::Type::getInt32Ty(ctx));
//// 
//// 		llvm::FunctionType* printf_type =
//// 			llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx), printf_arg_types, true);
//// 
//// 		llvm::Function *func = llvm::Function::Create(printf_type, llvm::Function::ExternalLinkage,
////         llvm::Twine("printf"),mod);
//// 		func->setCallingConv(llvm::CallingConv::C);
//// 	return func;
//// }
////
//////get a printf function
////Function* Get_print()
////{
////	LLVMContext& ctx = llvm::getGlobalContext();
////	Module* mod = new Module("test",ctx);
//////	constant* c = mod->getorinsertfunction("printf");
////	Function *printf_func = printf_prototype(ctx, mod);	
////	printf_func->setCallingConv(CallingConv::C);
////	return printf_func;
////}
//
//  Value*createInstr_SCEV (const SCEV *V, CallInst *calledInstr, IRBuilder<> &builder ) {
//      /*find the kind of scev, 
//        then based on the values used, try to map it to function argument, then find the value of the arguemnt from
//        calling function and use that.  Number to value mapping*/
//
//      unsigned scevT =   (V->getSCEVType());
//      Instruction::BinaryOps optype=Instruction::Add;
//
//      switch (static_cast<SCEVTypes>(scevT) ) {
//          case scUnknown: {
//                              const SCEVUnknown *LU = cast<SCEVUnknown>(V);
//                              const Value *LV = LU->getValue();
//                              errs()<<"\n Unknown Value::"<<*LV;
//                              if (const Argument *LA = dyn_cast<Argument>(LV)) {
//                                  errs() <<"\n argument::"<<*LA<<"\t argument number is ::"<<LA->getArgNo();
//                                  //getCalledFunction getFunction
//
//                                  int n=  LA->getArgNo();
//                                  Value *sentArg = calledInstr->getArgOperand (n ) ;
//                                  errs()<<"\n Value of arg from calling func ::"<< *sentArg;
//                                  return sentArg;
//                              }
//                              break;//TODO handle below cases
//                              if (const Instruction *LInst = dyn_cast<Instruction>(LV)) {
//                                  errs() <<"\n Error is instruction Not suppoerted::"<<*LInst;
//                                  unsigned LNumOps = LInst->getNumOperands();
//                                  errs() <<"\n num ops::"<<LNumOps;
//                                  return  nullptr;
//                              }
//                              if (bool LIsPointer = LV->getType()->isPointerTy()){
//                                  errs() <<"\n Error Pointer type bound not handled::"<<LIsPointer;
//                                  return nullptr;
//                              }
//                          }
//                          break   ;
//          case scConstant: {
//                               const SCEVConstant *LC = cast<SCEVConstant>(V);
//                               ConstantInt *constVal = LC->getValue();
//                               errs()<<"\n const val::"<<*constVal;
//                               return constVal;
//                           }
//                           break;
//                           break;
//          case scAddExpr:  optype = Instruction::Add;
//          case scMulExpr:  if (optype !=Instruction::Add) optype = Instruction::Mul;
//                           {
//                               const SCEVNAryExpr *NAry = cast<SCEVNAryExpr>(V);
//                               const char *OpStr = nullptr;
//                               /*switch (NAry->getSCEVType()) {
//                                 case scAddExpr: OpStr = " + "; break;
//                                 case scMulExpr: OpStr = " * "; break;
//                                 case scUMaxExpr: OpStr = " umax "; break;
//                                 case scSMaxExpr: OpStr = " smax "; break;
//                                 }*/
//                               const SCEVNAryExpr *LC = cast<SCEVNAryExpr>(V);
//                               errs() <<"\n scev nary::"<<*LC;
//
//                               // Lexicographically compare n-ary expressions.
//                               unsigned LNumOps = LC->getNumOperands();
//                               if (LNumOps == 2) {
//                                   const SCEV *scevA = LC->getOperand(0);
//                                   const SCEV *scevB = LC->getOperand(1);
//                                   Value *x = createInstr_SCEV(scevA, calledInstr, builder);
//                                   Value *y =createInstr_SCEV(scevB, calledInstr, builder) ;
//                                   if (x!= nullptr && y!= nullptr ) {
//                                       errs()<<"\n got operand instructions as ::"<<*x<<"\n and::"<<*y;
//                                       if (x->getType() != y->getType())
//                                           y = builder.CreateSExt(y, x->getType());
//                                       Value* tmp = builder.CreateBinOp(optype, x, y, "tmp");
//                                       errs()<<"\n got final instr::"<<*tmp;
//                                       return tmp;
//                                   }
//                               }
//
//                               // for (unsigned i = 0; i != LNumOps; ++i) {
//                               //     const SCEV *recV = LC->getOperand(i);
//                               //     errs()<<"\n op::"<<*(recV);
//                               //     printSCEVInfo(recV);
//                               // }
//                           }
//                           break;
//
//          case scUDivExpr: {
//                               const SCEVUDivExpr *LC = cast<SCEVUDivExpr>(V);
//
//                               errs()<<"\n udiv expr::"<<*LC<<"\n lhs as::"<<*(LC->getLHS());
//                           }
//                           break;
//
//          case scTruncate:{
//                              const SCEVCastExpr *LC = cast<SCEVCastExpr>(V);
//                              const SCEV *recV = LC->getOperand();
//                              errs()<<"\n sign truncate"<<*(recV);
//                              Value *v =  createInstr_SCEV(recV, calledInstr, builder );
//                              Value *truncV = builder.CreateTrunc(v, v->getType() );
//                                errs()<<"and the instruction is ::"<<*truncV;
//                              return truncV;
//                          }
//          case scZeroExtend:{
//                                const SCEVCastExpr *LC = cast<SCEVCastExpr>(V);
//                                const SCEV *recV = LC->getOperand();
//                                errs()<<"\n sign zero extend::"<<*(recV);
//                                Value *v =  createInstr_SCEV(recV, calledInstr, builder );
//                                Value *signZV = builder.CreateZExt(v, v->getType() );
//                                errs()<<"and the instruction is ::"<<*signZV;
//                                return signZV;
//                            }
//          case scSignExtend: {
//                                 const SCEVCastExpr *LC = cast<SCEVCastExpr>(V);
//
//                                 // Compare cast expressions by operand.
//                                 const SCEV *recV = LC->getOperand();
//                                 errs()<<"\n sign extend::"<<*(recV);
//                                 Value *v =  createInstr_SCEV(recV, calledInstr, builder );
//                                 Value *signV = builder.CreateSExt(v, v->getType() );
//                                errs()<<"and the instruction is ::"<<*signV;
//                                 return signV;
//                             }
//                             break;
//          case scAddRecExpr: errs()<<"\n recr expr"; break;
//                             //{                       const SCEVAddRecExpr *LA = cast<SCEVAddRecExpr>(V);
//                             //
//
//                             //                       // Compare addrec loop depths.
//                             //                       const Loop *LLoop = LA->getLoop();
//                             //                       unsigned LDepth = LLoop->getLoopDepth();
//
//                             //                       // Addrec complexity grows with operand count.
//                             //                       unsigned LNumOps = LA->getNumOperands();
//                             //                       errs()<<"\n addrecexpr::"<<*LA<<"\n lloop::"<< *LLoop<<"\n LDepth::"<<LDepth;
//
//                             //                       // Lexicographically compare.
//                             //                       for (unsigned i = 0; i != LNumOps; ++i) {
//                             //                           const SCEV *recV = LA->getOperand(i);
//                             //                           errs()<<"\n op::"<<*(recV);
//                             //                           printSCEVInfo(recV);
//                             //                       }
//
//                             //                   }
//          case scCouldNotCompute:
//                             errs()<<"\n Could not compute";
//      }
//
//      return nullptr;
//  }
///*
//  void construct_bound_expr (const SCEV *V, expression_tree &expression, bool is_left) {
//
//      unsigned scevT =   (V->getSCEVType());
//      switch (static_cast<SCEVTypes>(scevT) ) {
//          case scUnknown: {
//                              const SCEVUnknown *LU = cast<SCEVUnknown>(V);
//
//                              // Sort SCEVUnknown values with some loose heuristics. TODO: This is
//                              // not as complete as it could be.
//                              const Value *LV = LU->getValue();
//                              errs()<<"\n Unknown Value::"<<*LV;
//
//                              // Order pointer values after integer values. This helps SCEVExpander
//                              // form GEPs.
//                              if (bool LIsPointer = LV->getType()->isPointerTy()){
//                                  errs() <<"\n Error Pointer type bound not handled::"<<LIsPointer;
//                                  return ;
//                              }
//
//                              // Compare getValueID values.
//                              if (  LV->getValueID()) {
//                                  errs() << "\n value id not handled ::"<<LV->getValueID();
//                                  return;
//                                  }
//
//                              // Sort arguments by their position.
//                              if (const Argument *LA = dyn_cast<Argument>(LV)) {
//                                  errs() <<"\n argument::"<<*LA<<"\t argument number is ::"<<LA->getArgNo();
//                                  //getCalledFunction getFunction
//
//                                  int n=  LA->getArgNo();
//                                  expression.add_new_node(node_kind_type::ARGUMENT,&n , is_left);
//                                  return;
//                              }
//
//                              // For instructions, compare their loop depth, and their operand
//                              // count.  This is pretty loose.
//                              if (const Instruction *LInst = dyn_cast<Instruction>(LV)) {
//                                  errs() <<"\n Error is instruction Not suppoerted::"<<*LInst;
//                                  unsigned LNumOps = LInst->getNumOperands();
//                                  errs() <<"\n num ops::"<<LNumOps;
//                                  return;
//                              }
////dyn_cast<GlobalValue>(V)
//                              // Compare the number of operands.
//                          }
//                          break   ;
//          case scConstant: {
//                               const SCEVConstant *LC = cast<SCEVConstant>(V);
//
//                               // Compare constant values.
//                               const APInt &LA = LC->getAPInt();
//                                  expression.add_new_node(node_kind_type::CONST   ,LC , is_left);
//                               errs()<<"\n const val::"<<*LC<<"\t apiint::"<<LA;
//                           }
//                           break;
//          case scAddRecExpr: {
//                                 const SCEVAddRecExpr *LA = cast<SCEVAddRecExpr>(V);
//
//                                 // Compare addrec loop depths.
//                                 const Loop *LLoop = LA->getLoop();
//                                 unsigned LDepth = LLoop->getLoopDepth();
//
//                                 // Addrec complexity grows with operand count.
//                                 unsigned LNumOps = LA->getNumOperands();
//                                 errs()<<"\n addrecexpr::"<<*LA<<"\n lloop::"<< *LLoop<<"\n LDepth::"<<LDepth;
//
//                                 // Lexicographically compare.
//                                 for (unsigned i = 0; i != LNumOps; ++i) {
//                                     const SCEV *recV = LA->getOperand(i);
//                                     errs()<<"\n op::"<<*(recV);
//                                     printSCEVInfo(recV);
//                                 }
//
//                             }
//                             break;
//          case scAddExpr:
//          case scMulExpr:
//          case scSMaxExpr:
//          case scUMaxExpr: {
//                               const SCEVNAryExpr *LC = cast<SCEVNAryExpr>(V);
//                               errs() <<"\n scev nary::"<<*LC;
//
//                               // Lexicographically compare n-ary expressions.
//                               unsigned LNumOps = LC->getNumOperands();
//
//                               for (unsigned i = 0; i != LNumOps; ++i) {
//                                     const SCEV *recV = LC->getOperand(i);
//                                     errs()<<"\n op::"<<*(recV);
//                                     printSCEVInfo(recV);
//                               }
//                           }
//                           break;
//
//          case scUDivExpr: {
//                               const SCEVUDivExpr *LC = cast<SCEVUDivExpr>(V);
//
//                               errs()<<"\n udiv expr::"<<*LC<<"\n lhs as::"<<*(LC->getLHS());
//                           }
//                           break;
//
//          case scTruncate:
//          case scZeroExtend:
//          case scSignExtend: {
//                                 const SCEVCastExpr *LC = cast<SCEVCastExpr>(V);
//
//                                 // Compare cast expressions by operand.
//                                     const SCEV *recV = LC->getOperand();
//                                     errs()<<"\n sign extend::"<<*(recV);
//                                     printSCEVInfo(recV);
//                             }
//                             break;
//
//          case scCouldNotCompute:
//                             errs()<<"\n Could not compute";
//      }
//
//  }
//  */
////void LoopAccessInfo::collectStridedAccess(Value *MemAccess) 
////getStrideFromPointer
//  void printSCEVInfo (const SCEV *V) {
//
//      unsigned scevT =   (V->getSCEVType());
//      switch (static_cast<SCEVTypes>(scevT) ) {
//          case scUnknown: {
//                              const SCEVUnknown *LU = cast<SCEVUnknown>(V);
//
//                              // Sort SCEVUnknown values with some loose heuristics. TODO: This is
//                              // not as complete as it could be.
//                              const Value *LV = LU->getValue();
//                              errs()<<"\n Unknown Value::"<<*LV;
//
//                              // Order pointer values after integer values. This helps SCEVExpander
//                              // form GEPs.
//                              if (bool LIsPointer = LV->getType()->isPointerTy())
//                                  errs() <<"\n is pointer type ::"<<LIsPointer;
//
//                              // Compare getValueID values.
//                              if (  LV->getValueID())
//                                  errs() << "\n L value id ::"<<LV->getValueID();
//
//                              // Sort arguments by their position.
//                              if (const Argument *LA = dyn_cast<Argument>(LV)) {
//                                  errs() <<"\n argument::"<<*LA<<"\t argument number is ::"<<LA->getArgNo();
//                                  //getCalledFunction getFunction
//                              }
//
//                              // For instructions, compare their loop depth, and their operand
//                              // count.  This is pretty loose.
//                              if (const Instruction *LInst = dyn_cast<Instruction>(LV)) {
//                                  errs() <<"\n is instruction ::"<<*LInst;
//                                  unsigned LNumOps = LInst->getNumOperands();
//                                  errs() <<"\n num ops::"<<LNumOps;
//                              }
////dyn_cast<GlobalValue>(V)
//                              // Compare the number of operands.
//                          }
//                          break   ;
//          case scConstant: {
//                               const SCEVConstant *LC = cast<SCEVConstant>(V);
//
//                               // Compare constant values.
//                               const APInt &LA = LC->getAPInt();
//                               errs()<<"\n const val::"<<*LC<<"\t apiint::"<<LA;
//                           }
//                           break;
//          case scAddRecExpr: {
//                                 const SCEVAddRecExpr *LA = cast<SCEVAddRecExpr>(V);
//
//                                 // Compare addrec loop depths.
//                                 const Loop *LLoop = LA->getLoop();
//                                 unsigned LDepth = LLoop->getLoopDepth();
//
//                                 // Addrec complexity grows with operand count.
//                                 unsigned LNumOps = LA->getNumOperands();
//                                 errs()<<"\n addrecexpr::"<<*LA<<"\n lloop::"<< *LLoop<<"\n LDepth::"<<LDepth;
//
//                                 // Lexicographically compare.
//                                 for (unsigned i = 0; i != LNumOps; ++i) {
//                                     const SCEV *recV = LA->getOperand(i);
//                                     errs()<<"\n op::"<<*(recV);
//                                     printSCEVInfo(recV);
//                                 }
//
//                             }
//                             break;
//          case scAddExpr:
//          case scMulExpr:
//          case scSMaxExpr:
//          case scUMaxExpr: {
//                               const SCEVNAryExpr *LC = cast<SCEVNAryExpr>(V);
//                               errs() <<"\n scev nary::"<<*LC;
//
//                               // Lexicographically compare n-ary expressions.
//                               unsigned LNumOps = LC->getNumOperands();
//
//                               for (unsigned i = 0; i != LNumOps; ++i) {
//                                     const SCEV *recV = LC->getOperand(i);
//                                     errs()<<"\n op::"<<*(recV);
//                                     printSCEVInfo(recV);
//                               }
//                           }
//                           break;
//
//          case scUDivExpr: {
//                               const SCEVUDivExpr *LC = cast<SCEVUDivExpr>(V);
//
//                               errs()<<"\n udiv expr::"<<*LC<<"\n lhs as::"<<*(LC->getLHS());
//                           }
//                           break;
//
//          case scTruncate:
//          case scZeroExtend:
//          case scSignExtend: {
//                                 const SCEVCastExpr *LC = cast<SCEVCastExpr>(V);
//
//                                 // Compare cast expressions by operand.
//                                     const SCEV *recV = LC->getOperand();
//                                     errs()<<"\n sign extend::"<<*(recV);
//                                     printSCEVInfo(recV);
//                             }
//                             break;
//
//          case scCouldNotCompute:
//                             errs()<<"\n Could not compute";
//      }
//
//  }
//  const SCEV *letsComputeStride(Value *Ptr, ScalarEvolution *SE, Loop *Lp) {
//      errs() << "\n Computing Strid";
//      auto PtrTy = dyn_cast<PointerType>(Ptr->getType());
//      if (!PtrTy || PtrTy->isAggregateType()) {
//          errs() <<"\n not of poitner type::";
//          return nullptr;
//      }
//      Value *OrigPtr = Ptr;
//      int64_t PtrAccessSize = 1;
//      Ptr = stripGetElementPtr(Ptr, SE, Lp);
//      const SCEV *V =  SE->getSCEV(Ptr);
//      errs()   <<"\n ptr::"<<*Ptr<<" \n OrigPtr::"<<*OrigPtr;
//      errs() << "\n SCEV ::"<< *V;
//      if (Ptr != OrigPtr) //Strip off casts
//          while (const SCEVCastExpr *C = dyn_cast<SCEVCastExpr>(V))
//              V = C->getOperand();
//      //printSCEVInfo(V);
//
//      const SCEVAddRecExpr *S = dyn_cast<SCEVAddRecExpr>(V); 
//      if (!S) {
//          errs() << "\n oops not scevaddrecexpr typ";
//          return nullptr;
//      }
//      errs() <<"\n got SCEV add rec expr as ::"<< *S;
//      V = S->getStepRecurrence(*SE);
//      if (!V) {
//          errs()<< "\n OOps No step recurrence value";
//          return nullptr;
//      }
//      //class SCEVAddRecExpr : public SCEVNAryExpr :: has all important info
//      errs() <<"\n got step recurence as ::"<<*V;
//      return V;
//  }
//void runStrideComputation(Loop *L, AliasAnalysis *AA,
//                                        LoopInfo *LI, DominatorTree *DT,
//                                        TargetLibraryInfo *TLI,
//                                        ScalarEvolution *SE) {
//
//
//std::set<unsigned int> setOf_memory_arguments;
//    for (BasicBlock *BB : L->blocks()) 
//        for (Instruction &I : *BB)  {
//            if (thisModule == nullptr)
//            thisModule = I.getModule();
//                errs() << "\n "<< I;
//        }
//    errs()<<"\n=========================================";
//    for (BasicBlock *BB : L->blocks()) {
//        for (Instruction &I : *BB) {
//            GetElementPtrInst *gep;
//            if (gep = dyn_cast<GetElementPtrInst>(&I)) {
//                errs() <<"\n found gep::"<<*gep;
//                for (int i=0; i< gep->getNumOperands();i++) {
//                    Value *mem = gep->getOperand(i);
//                    errs() << "\n secondop is ::"<<*mem ;
//                    if  (const Argument *arg = dyn_cast<Argument>(mem)){
//                        unsigned int argn = arg->getArgNo();
//                        errs() <<"\n arguemtn numne ois ::"<<argn;
//                        setOf_memory_arguments.insert(argn);
//                    }
//                }
//            }
//            if (I.mayReadFromMemory() ) {
//                auto *Ld = dyn_cast<LoadInst>(&I);
//                if (!Ld || (!Ld->isSimple() ))
//                    continue;
//                LoadInst *Li = dyn_cast<LoadInst> (Ld);
//                Value *Ptr  = Li->getPointerOperand();
//                errs() << "\n Load ptr is ::"<<*Ptr;
//                errs() << "\n first op is ::"<<(I.getNumOperands());
//                for (int i=0; i< I.getNumOperands();i++) {
//                errs() << "\n secondop is ::"<<*Li->getOperand(i);
//                }
//                //errs() << "\n secondop is ::"<<*Li->getOperand(1);
//                //:getPtrStride
//                const SCEV  *stride = letsComputeStride(Ptr, SE, L);
//                if (!stride) continue;
//                errs() << "\n Stride is :"<< *stride;
//            }
//        }
//    }
//
//  errs() <<"\n is loop invariant back edge count?::"<<(SE->hasLoopInvariantBackedgeTakenCount(L));
////  errs() <<"\n backedgetakeninfo::"<<*(SE->getBackedgeTakenInfo());
//  if (SE->hasLoopInvariantBackedgeTakenCount(L)) {
//      const SCEV *UB = SE->getBackedgeTakenCount(L);
//      errs() << "\n bound::"<< *UB << "\n and type is ::"<<*(UB->getType())<<"\n and scevtype is :"<<UB->getSCEVType();
//      mem_stride_info *stride = new mem_stride_info; 
//      stride->mem_argNums = setOf_memory_arguments;
//      stride->upperBound = UB;
//      stride->stepRecr = NULL;
////      BlockT * bb = L->getBlocks()[0]->getParent();
//      Function *f = L->getBlocks()[0]->getParent();//bb->getParent();
//      functionStrideMap.insert(std::pair< Function*, mem_stride_info* > (f,stride ) );
//
//
//      printSCEVInfo(UB);
//      //construct_bound_expr(UB);
//  }
//
//  /*if (const SCEVAddExpr *Sum = dyn_cast<SCEVAddExpr>(UB)) {
//    // If Delta is a sum of products, we may be able to make further progress.
//    for (unsigned Op = 0, Ops = Sum->getNumOperands(); Op < Ops; Op++) {
//      const SCEV *Operand = Sum->getOperand(Op);
//      errs() <<"\n op of scev is ::"<<*Operand;
//      errs()_<<"\n optype is ::"<<Operand->getSCEVType();
//      if (Operand->getSCEVType() == scUMaxExpr) 
//          errs() <<"\n umax expr is ::"<< SE.getUMaxExpr(Operands); 
//    }
//  */
//    /*
//    const SCEV *V =  UB  ;
//    while (const SCEVCastExpr *C = dyn_cast<SCEVCastExpr>(V))
//        V = C->getOperand();
//    errs() <<"\n scev::"<<*V;
//    for (unsigned Op = 0, Ops = V->getNumOperands(); Op < Ops; Op++)  {
//      const SCEV *Operand = V->getOperand(Op);
//      errs() <<"\n operand::"<< *Operand;
//
//    }*/
//  
//}
//bool AnalyzeThisLoop(Loop *L, AliasAnalysis *AA,
//                                        LoopInfo *LI, DominatorTree *DT,
//                                        TargetLibraryInfo *TLI,
//                                        ScalarEvolution *SE, bool DeleteAST) {
//  bool Changed = false;
//
//  assert(L->isLCSSAForm(*DT) && "Loop is not in LCSSA form.");
//
//  errs() << "\n Starting Loop Stride Analysis for loop \n"<<*L;
//  runStrideComputation(L,AA, LI, DT, TLI, SE );
//  return false;
//
//  //AliasSetTracker *CurAST = collectAliasInfoForLoop(L, LI, AA);
//
//  // Get the preheader block to move instructions into...
//  BasicBlock *Preheader = L->getLoopPreheader();
//
//
//  // We want to visit all of the instructions in this loop... that are not parts
//  // of our subloops (they have already had their invariants hoisted out of
//  // their loop, into this loop, so there is no need to process the BODIES of
//  // the subloops).
//  //
//  // Traverse the body of the loop in depth first order on the dominator tree so
//  // that we are guaranteed to see definitions before we see uses.  This allows
//  // us to sink instructions in one pass, without iteration.  After sinking
//  // instructions, we perform another pass to hoist them out of the loop.
//  //
//    Changed |= sinkRegion(DT->getNode(L->getHeader()), AA, LI, DT, TLI, L);
//    //                      CurAST, &SafetyInfo);
//  
//
//  return Changed;
//}
//
///// Walk the specified region of the CFG (defined by all blocks dominated by
///// the specified block, and that are in the current loop) in reverse depth
///// first order w.r.t the DominatorTree.  This allows us to visit uses before
///// definitions, allowing us to sink a loop body in one pass without iteration.
/////
//bool sinkRegion(DomTreeNode *N, AliasAnalysis *AA, LoopInfo *LI,
//                      DominatorTree *DT, TargetLibraryInfo *TLI, Loop *CurLoop){
//
//  // Verify inputs.
//  assert(N != nullptr && AA != nullptr && LI != nullptr && DT != nullptr &&
//         CurLoop != nullptr  &&
//         "Unexpected input to sinkRegion");
//
//      //errs() << "inside sinkregion             ";
//  BasicBlock *BB = N->getBlock();
//  BasicBlock *H = BB;
//  // If this subregion is not in the top level loop at all, exit.
//  if (!CurLoop->contains(BB))
//    return false;
//
//  // We are processing blocks in reverse dfo, so process children first.
//  bool Changed = false;
//  for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; I++)
//        {
////  for (BasicBlock::iterator II = BB->end(); II != BB->begin();) {
//  //  Instruction &I = *--II;
//      errs() << "\n instruction :: " << *I ;
//      int num_ops = I->getNumOperands() -1;
//      errs() << " \n Num operands are ::"<<num_ops;
//      for (int i=0;i<num_ops;i++) {
//        errs()<<"\n operand is ::"<< *(I->getOperand(i));
//      }
//
//  }
//  BasicBlock *Incoming = nullptr, *Backedge = nullptr;
//  pred_iterator PI = pred_begin(H);
//  assert(PI != pred_end(H) &&
//         "Loop must have at least one backedge!");
//  Backedge = *PI++;
//  if (PI == pred_end(H)) return false  ;  // dead loop
//  Incoming = *PI++;
//  if (PI != pred_end(H)) return false  ;  // multiple backedges?
//
//  if (CurLoop->contains(Incoming)) {
//    if (CurLoop->contains(Backedge))
//      return false  ;
//    std::swap(Incoming, Backedge);
//  } else if (!CurLoop->contains(Backedge))
//    return false  ;
//
//  // Loop over all of the PHI nodes, looking for a canonical indvar.
//  for (BasicBlock::iterator I = H->begin(); isa<PHINode>(I); ++I) {
//	errs() << "\n Looping over phi nodes::"<<*I;
//    PHINode *PN = cast<PHINode>(I);
//    if (ConstantInt *CI =
//        dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
//      if (CI->isNullValue())
//        if (Instruction *Inc =
//            dyn_cast<Instruction>(PN->getIncomingValueForBlock(Backedge)))
//          if (Inc->getOpcode() == Instruction::Add &&
//                Inc->getOperand(0) == PN)
//            if (ConstantInt *CI = dyn_cast<ConstantInt>(Inc->getOperand(1)))
//              if (CI->equalsInt(1))
//                return false;
//  }
//  return Changed;
//}
//};
//}
//
//char MyLoopInfo::ID = 0;
//static RegisterPass<MyLoopInfo>
//Z("myLoopInfo", "Get Loop Info pass by Hello World ");
//
//
//
//
//
//STATISTIC(NumLoopNests, "Number of loop nests");
//STATISTIC(NumPerfectLoopNests, "Number of perfect loop nests");
//STATISTIC(NumIndependentMemOps, "Number of independent memory operations");
//STATISTIC(NumAmbiguousDependentMemOps, "Number of ambiguous dependent memory operations");
//STATISTIC(NumDirectionalDependentMemOps, "Number of directional dependent memory operations");
//STATISTIC(NumInterestingLoops, "Number of loops with a nontrivial direction matrix");
//
//namespace CS1745
//{
//
///* A single dependence */
//struct DependenceDirection
//{
//	char direction; // <,=,>, or *
//	int distance; //if exists, nonzero
//	
//	DependenceDirection(): direction('*'), distance(0) {}
//};
//
///*
// A class that represents a dependence matrix for a loop.  This is the union
// of all valid direction vectors of all dependences.
// */
//class DependenceMatrix
//{
//	//implement, feel free to change any data structures
//public:
//	DependenceMatrix();
//	DependenceMatrix(int n);
//	void add(const std::vector<DependenceDirection>& vec);
//	//more ?
//	void clear();	
//	void print(	std::ostream& out);
//	bool isTrivial(); //return true if empty (no dependence) or full (all dependencies)
//};
//
//class LoopDependence : public LoopPass
//{
//	std::ofstream file;
//public:
//	static char ID;
//
//	LoopDependence() : 
//		LoopPass(ID)
//	{
//		std::string name = "DEPINFO";
//	}
//
//	~LoopDependence()
//	{
//	}
//
//	virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
//
//	virtual void getAnalysisUsage(AnalysisUsage &AU) const
//	{
//		AU.setPreservesAll();	// We don't modify the program, so we preserve all analyses
//		/*AU.addRequiredID(LoopSimplifyID);
//		AU.addRequired<LoopInfo>();
//		AU.addRequiredTransitive<AliasAnalysis>();
//		AU.addPreserved<AliasAnalysis>();*/
//	}
//
//private:
//	// Various analyses that we use...
//	AliasAnalysis *AA; // Current AliasAnalysis information
//	LoopInfo *LI; // Current LoopInfo
//
//
//};
//
//
//
//// Compute the dependence matrix of a loop.  Only consider perfectly
//// nested, single block loops with easily analyzed array accesses.
//bool LoopDependence::runOnLoop(Loop *L, LPPassManager &LPM)
//{
//      errs() << "Hello We called runonloop :: \n" ;
//
//
//	//now check that these loops are perfectly nested inside one another
//	BasicBlock *body;
//
//	
//	//go through the basic block and make sure only loads and stores
//	//access memory
//	for (BasicBlock::iterator I = body->begin(), E = body->end(); I != E; I++)
//	{
//		switch (I->getOpcode())
//		{
//			case Instruction::Invoke:
//			case Instruction::VAArg:
//			case Instruction::Call:
//				return false; //can't handle these		
//		}
//	}
//	
//	return false;
//}
//
//
//char LoopDependence::ID = 0;
//RegisterPass<LoopDependence> A("loop-dep", "Loop Dependence");
//}
/*
    enum stride_info_op_type {ADD, MULTIPLY, DIVIDE, POINTER_DEREFERENCE,INVALID }
    class strideInfo {
        unsigned int memArgument_num;
        class  expression_nodeType {
            int arg_num;
            GlobalVariable *globalVar ;//CAN ALSO BE A CONSTNAT
            bool isFunc_arg;
            stride_info_op_type operation;
            public:
            expression_nodeType() {
                arg_num = -1;
                globalVar = NULL;
                isFunc_arg = false;
                operation = INVALID;
            }
            expression_nodeType(const expression_nodeType &t) :
            arg_num(t.arg_num),  globalVar(t.globalVar), isFunc_arg(t.isFunc_arg), operation(t.operation);
            void set_arg_num(int n) { arg_num = n; isFunc_arg = 1;}
        };
        std::list<expression_nodeType> strideComputationExpression;
        public:
        strideInfo(int mem_argument ){
            memArgument_num = mem_argument;
        }
        void set_Expr_node(bool is_arg, int arg_num, GlobalVariable* gv=NULL,int op=-1 ){
            expression_nodeType t_node;
            t_node.isFunc_arg = is_arg;
            if (is_arg)
            t_node.arg_num  = arg_num;
            t_node.globalVar = gv;
            t_node.operation = op;
            strideComputationExpression.push_back(t_node ) ;
        }
        void print() {
            std::list<expression_nodeType>::iterator it;
            std::cout<<"\n Mem argument is::"<<memArgument_num;
            for (it=strideComputationExpression.begin(); it != strideComputationExpression.end() ; ++it ){
                std::cout<<"\n Variable::"<< (it->isFunc_arg ? it->arg_num :-1 ) <<"\t operation::  "<<it->operation;
            }
        }
    };
    */
    
    /*

    enum node_kind_type {ARGUMENT, GLOBAL, CONST, ADD, MULT, DIV, INVALID };
    class  expression_tree_node {
        friend class expression_tree;
        expression_tree_node *left;
        expression_tree_node *right;
        expression_tree_node * parent;
        node_kind_type nkind;
        int *argument_num;
        GlobalValue *globalVar;
        SCEVConstant *constInt;
        public:
        expression_tree_node(){
            left= NULL;
            right = NULL;
            parent = NULL;
            nkind = INVALID;
            argument_num = NULL;
            globalVar = NULL;
            constInt = NULL;
        }
        expression_tree_node *expression_tree_node_add_node(node_kind_type t,const void *val, bool is_left ){

            expression_tree_node *new_node = new expression_tree_node();
            new_node->nkind = t;
            new_node->parent = this;
            switch (t){
                case ARGUMENT: 
                    new_node->argument_num = new int      ;
                    new_node->argument_num[0] = *((int*)val)   ;
                    break;
                case GLOBAL:
                    new_node->globalVar = (GlobalValue *)val ;
                    break;
                case CONST:
                    new_node->constInt = (SCEVConstant *)val;
            }
            if (is_left )
                left = new_node;
            else
                right = new_node;
            return new_node;
        }
        node_kind_type getNodeKind() { return nkind;}
        expression_tree_node *getLeft() {return left;}
        expression_tree_node *getRight() {return right;}
        void * getValue() {

            switch (nkind){
                case ARGUMENT: 
                    return    argument_num ;
                    break;
                case GLOBAL:
                    return globalVar  ;
                    break;
                case CONST:
                    return    constInt ;
            }
            return NULL;
        }
    };
    class expression_tree {
        expression_tree_node *head;
        expression_tree_node *current;
        public:
        expression_tree () {
            head = NULL;
            current = NULL;
        }
        expression_tree_node *get_current () {return current;}
        expression_tree_node *get_head    () {return head   ;}
        void add_new_node(node_kind_type t,const void *val, bool is_left) {
            if (head == NULL){
                head = new expression_tree_node;
                current = head;
            }
            current = current->expression_tree_node_add_node(t, val, is_left );
        }
        void moveup() { current = current->parent;}
    };
    struct strideInfo {
        int mem_arg_num;
        expression_tree exp_compute_bound;
    };
  std::map<const Function *, strideInfo*> functionStride_map;
 */
 

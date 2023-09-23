#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTFunctions.h"
#include "JellyCore/Allocator.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/IRBuilder.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Support.h>

#warning TODO: Always default initialize the memory of any value declaration if there is no initializer expression!

// TODO: The dereference expression in the following code sample is not load correctly before the icmp instruction!
// func replaceOccurenciesOf(string: String*, character: UInt8, replacement: UInt8) -> Void {
//     var index: Int = 0
//     while index < string.length {
//         var cursor: UInt8* = string.memory + sizeof(UInt8) * index
//         if *cursor == character { // This is making a "icmp eq i8* %16, i8 %1" but should be a "icmp eq i8 ..."!
//             *cursor = replacement
//         }
//
//         index = index + 1
//     }
// }

// TODO: Rename this to LLVMBackend
struct _IRBuilder {
    AllocatorRef allocator;
    ASTContextRef astContext;
    StringRef buildDirectory;
    LLVMContextRef context;
    LLVMBuilderRef builder;
    IRModuleRef module;
};

struct _IRModule {
    LLVMModuleRef module;
    Bool isVerified;
};

// TODO: Add correct implementation for enumeration type and cases
// TODO: Remove initializers from backend and apply ast substitutions!

static inline void _IRBuilderBuildEntryPoint(IRBuilderRef builder, ASTModuleDeclarationRef module);
static inline void _IRBuilderBuildTypes(IRBuilderRef builder, ASTModuleDeclarationRef module);
static inline void _IRBuilderBuildGlobalVariables(IRBuilderRef builder, ASTModuleDeclarationRef module);
static inline void _IRBuilderBuildGlobalVariable(IRBuilderRef builder, ASTValueDeclarationRef declaration);
static inline void _IRBuilderBuildEnumerationElements(IRBuilderRef builder, ASTEnumerationDeclarationRef declaration);
static inline void _IRBuilderBuildFunctionSignature(IRBuilderRef builder, ASTFunctionDeclarationRef declaration);
static inline void _IRBuilderBuildFunctionBody(IRBuilderRef builder, ASTFunctionDeclarationRef declaration);
static inline void _IRBuilderBuildForeignFunctionSignature(IRBuilderRef builder, ASTFunctionDeclarationRef declaration);
static inline void _IRBuilderBuildInitializerSignature(IRBuilderRef builder, ASTInitializerDeclarationRef declaration);
static inline void _IRBuilderBuildInitializerBody(IRBuilderRef builder, ASTStructureDeclarationRef structure,
                                                  ASTInitializerDeclarationRef declaration);
static inline void _IRBuilderBuildLocalVariable(IRBuilderRef builder, LLVMValueRef function, ASTValueDeclarationRef declaration);
static inline void _IRBuilderBuildBlock(IRBuilderRef builder, LLVMValueRef function, ASTBlockRef block);
static inline void _IRBuilderBuildStatement(IRBuilderRef builder, LLVMValueRef function, ASTNodeRef node);
static inline void _IRBuilderBuildIfStatement(IRBuilderRef builder, LLVMValueRef function, ASTIfStatementRef statement);
static inline void _IRBuilderBuildLoopStatement(IRBuilderRef builder, LLVMValueRef function, ASTLoopStatementRef statement);
static inline void _IRBuilderBuildSwitchStatement(IRBuilderRef builder, LLVMValueRef function, ASTSwitchStatementRef statement);
static inline void _IRBuilderBuildControlStatement(IRBuilderRef builder, LLVMValueRef function, ASTControlStatementRef statement);
static inline void _IRBuilderBuildExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression);
static inline void _IRBuilderBuildConstantExpression(IRBuilderRef builder, ASTExpressionRef constant);

static inline LLVMValueRef _IRBuilderBuildBinaryExpression(IRBuilderRef builder, LLVMValueRef function, ASTFunctionDeclarationRef callee,
                                                           LLVMValueRef *arguments);

static inline LLVMValueRef _IRBuilderLoadExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression);

static inline LLVMTypeRef _IRBuilderGetIRType(IRBuilderRef builder, ASTTypeRef type);

static inline LLVMValueRef _IRBuilderBuildIntrinsic(IRBuilderRef builder, LLVMValueRef function, StringRef intrinsic,
                                                    LLVMValueRef *arguments, unsigned argumentCount, LLVMTypeRef resultType);

static inline LLVMValueRef _IRBuilderBuildLoad(IRBuilderRef Builder, ASTNodeRef Node, const Char* Name);

LLVMValueRef _IRBuilderGetConstantSizeOfType(IRBuilderRef builder, ASTTypeRef type);

LLVMValueRef _IRBuilderImplicitlyConvertValue(IRBuilderRef builder, LLVMValueRef function, LLVMValueRef value,
                                              ASTExpressionRef valueExpression, ASTTypeRef targetType);

IRBuilderRef IRBuilderCreate(AllocatorRef allocator, ASTContextRef context, StringRef buildDirectory) {
    IRBuilderRef builder    = (IRBuilderRef)AllocatorAllocate(allocator, sizeof(struct _IRBuilder));
    builder->allocator      = allocator;
    builder->astContext     = context;
    builder->buildDirectory = StringCreateCopy(allocator, buildDirectory);
    builder->context        = LLVMGetGlobalContext();
    builder->builder        = LLVMCreateBuilderInContext(builder->context);
    builder->module         = NULL;
    return builder;
}

void IRBuilderDestroy(IRBuilderRef builder) {
    if (builder->module) {
        LLVMDisposeModule(builder->module->module);
        AllocatorDeallocate(builder->allocator, builder->module);
    }

    LLVMDisposeBuilder(builder->builder);

    StringDestroy(builder->buildDirectory);
    AllocatorDeallocate(builder->allocator, builder);
}

IRModuleRef IRBuilderBuild(IRBuilderRef builder, ASTModuleDeclarationRef module) {
    assert(module->base.name);

    if (builder->module) {
        LLVMDisposeModule(builder->module->module);
        AllocatorDeallocate(builder->allocator, builder->module);
    }

    builder->module             = (IRModuleRef)AllocatorAllocate(builder->allocator, sizeof(struct _IRModule));
    builder->module->module     = LLVMModuleCreateWithNameInContext(StringGetCharacters(module->base.name), builder->context);
    builder->module->isVerified = false;

    // It would at least be better to pre create all known types and only use the prebuild types, same applies for literal values
    if (ASTArrayGetElementCount(module->sourceUnits) > 0) {
        ASTSourceUnitRef initialSourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, 0);
        LLVMSetSourceFileName(builder->module->module, StringGetCharacters(initialSourceUnit->filePath),
                              StringGetLength(initialSourceUnit->filePath));
    }

    _IRBuilderBuildTypes(builder, module);
    _IRBuilderBuildGlobalVariables(builder, module);

    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            // TODO: Also build the value for foreign and intrinsic functions!
            if (child->tag == ASTTagFunctionDeclaration) {
                _IRBuilderBuildFunctionSignature(builder, (ASTFunctionDeclarationRef)child);
            }

            if (child->tag == ASTTagForeignFunctionDeclaration) {
                _IRBuilderBuildForeignFunctionSignature(builder, (ASTFunctionDeclarationRef)child);
            }

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)child;
                ASTArrayIteratorRef iterator         = ASTArrayGetIterator(structure->initializers);
                while (iterator) {
                    ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)ASTArrayIteratorGetElement(iterator);
                    _IRBuilderBuildInitializerSignature(builder, initializer);
                    iterator = ASTArrayIteratorNext(iterator);
                }
            }
        }
    }

    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagLoadDirective) {
                continue;
            }

            switch (child->tag) {
            case ASTTagLoadDirective:
            case ASTTagImportDirective:
            case ASTTagIncludeDirective:
                continue;

            case ASTTagEnumerationDeclaration:
                _IRBuilderBuildEnumerationElements(builder, (ASTEnumerationDeclarationRef)child);
                break;

            case ASTTagFunctionDeclaration:
                _IRBuilderBuildFunctionBody(builder, (ASTFunctionDeclarationRef)child);
                break;

            case ASTTagForeignFunctionDeclaration:
                break;

            case ASTTagIntrinsicFunctionDeclaration:
                continue;

            case ASTTagStructureDeclaration: {
                ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)child;
                ASTArrayIteratorRef iterator         = ASTArrayGetIterator(structure->initializers);
                while (iterator) {
                    ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)ASTArrayIteratorGetElement(iterator);
                    _IRBuilderBuildInitializerBody(builder, structure, initializer);
                    iterator = ASTArrayIteratorNext(iterator);
                }
                break;
            }

            case ASTTagValueDeclaration:
                continue;

            case ASTTagTypeAliasDeclaration:
                continue;

            default:
                JELLY_UNREACHABLE("Invalid tag given for top level node!");
                break;
            }
        }
    }

    if (module->kind == ASTModuleKindExecutable) {
        _IRBuilderBuildEntryPoint(builder, module);
    }

    return builder->module;
}

void IRBuilderDumpModule(IRBuilderRef builder, IRModuleRef module, FILE *target) {
    assert(builder->module == module && module);

    Char *dump = LLVMPrintModuleToString(module->module);
    fprintf(target, "%s", dump);
    LLVMDisposeMessage(dump);
}

void IRBuilderVerifyModule(IRBuilderRef builder, IRModuleRef module) {
    assert(builder->module == module && module);

    if (module->isVerified) {
        return;
    }
    module->isVerified = true;

    Char *message  = NULL;
    LLVMBool error = LLVMVerifyModule(module->module, LLVMReturnStatusAction, &message);
    if (error) {
        if (message) {
            ReportErrorFormat("LLVM Error:\n%s\n", message);
        } else {
            ReportError("LLVM Module Verification failed");
        }
    }

    if (message) {
        LLVMDisposeMessage(message);
    }
}

void IRBuilderEmitObjectFile(IRBuilderRef builder, IRModuleRef module, StringRef fileName) {
    assert(builder->module == module && module);
    assert(builder->module->isVerified);

    Char *targetTriple = LLVMGetDefaultTargetTriple();
    Char *cpu          = LLVMGetHostCPUName();
    Char *features     = LLVMGetHostCPUFeatures();
    
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    LLVMTargetRef target = NULL;
    Char *message        = NULL;
    LLVMBool error       = LLVMGetTargetFromTriple(targetTriple, &target, &message);
    if (error) {
        if (message) {
            ReportErrorFormat("LLVM Error:\n%s\n", message);
            LLVMDisposeMessage(message);
        } else {
            ReportError("LLVM Target initialization failed");
        }

        return;
    }

    // TODO: Add configuration option to IRBuilder for LLVMCodeGenLevel
    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(target, targetTriple, cpu, features, LLVMCodeGenLevelNone, LLVMRelocDefault,
                                                           LLVMCodeModelDefault);
    LLVMTargetDataRef dataLayout = LLVMCreateTargetDataLayout(machine);

    LLVMSetTarget(builder->module->module, targetTriple);
    LLVMSetModuleDataLayout(builder->module->module, dataLayout);

    StringRef objectFilePath = StringCreateCopy(builder->allocator, builder->buildDirectory);
    StringAppendFormat(objectFilePath, "/%s.o", StringGetCharacters(fileName));

    FILE *objectFile = fopen(StringGetCharacters(objectFilePath), "w+");
    if (!objectFile) {
        ReportErrorFormat("Couldn't create object file at path: '%s'", StringGetCharacters(objectFilePath));
        // TODO: Dispose all LLVM references
        return;
    } else {
        fclose(objectFile);
    }

    error = LLVMTargetMachineEmitToFile(machine, builder->module->module, StringGetCharacters(objectFilePath), LLVMObjectFile, &message);
    if (error) {
        if (message) {
            ReportCriticalFormat("LLVM Error:\n%s\n", message);
            LLVMDisposeMessage(message);
        } else {
            ReportCritical("LLVM Error");
        }

        // TODO: Dispose all LLVM references
        return;
    }

    StringDestroy(objectFilePath);
    LLVMDisposeTargetData(dataLayout);
    LLVMDisposeTargetMachine(machine);
    LLVMDisposeMessage(targetTriple);
    LLVMDisposeMessage(cpu);
    LLVMDisposeMessage(features);
}

static inline void _IRBuilderBuildEntryPoint(IRBuilderRef builder, ASTModuleDeclarationRef module) {
    if (module->entryPoint) {
        assert(module->entryPoint->base.base.irValue);
        LLVMTypeRef entryPointParameterTypes[] = {LLVMInt32Type(), LLVMPointerType(LLVMInt8Type(), 0)};
        LLVMTypeRef entryPointType             = LLVMFunctionType(LLVMInt32Type(), entryPointParameterTypes, 2, false);
        LLVMValueRef entryPoint                = LLVMAddFunction(builder->module->module, "main", entryPointType);
        LLVMSetFunctionCallConv(entryPoint, LLVMCCallConv);
        LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(entryPoint, "entry");
        LLVMPositionBuilder(builder->builder, entryBB, NULL);
        
        LLVMTypeRef Type = _IRBuilderGetIRType(builder, module->entryPoint->base.base.type);
        LLVMBuildCall2(builder->builder, Type, (LLVMValueRef)module->entryPoint->base.base.irValue, NULL, 0, "");
        LLVMBuildRet(builder->builder, LLVMConstInt(LLVMInt32Type(), 0, true));
    }
}

static inline void _IRBuilderBuildTypes(IRBuilderRef builder, ASTModuleDeclarationRef module) {
    // Prebuild types which should be added to the stdlib soon...
    _IRBuilderGetIRType(builder, (ASTTypeRef)ASTContextGetStringType(builder->astContext));

    // Build structure signatures
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagStructureDeclaration) {
                ASTDeclarationRef declaration = (ASTDeclarationRef)child;
                declaration->base.irType      = LLVMStructCreateNamed(builder->context, StringGetCharacters(declaration->mangledName));
            }
        }
    }

    // Build function signatures, structure bodies, global variable types
    ArrayRef temporaryTypes = ArrayCreateEmpty(builder->allocator, sizeof(LLVMTypeRef), 8);
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ArrayRemoveAllElements(temporaryTypes, true);

            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagFunctionDeclaration || child->tag == ASTTagForeignFunctionDeclaration) {
                ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)child;
                for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
                    ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
                    LLVMTypeRef parameterType        = _IRBuilderGetIRType(builder, parameter->base.base.type);
                    assert(parameterType);
                    parameter->base.base.irType = parameterType;
                    ArrayAppendElement(temporaryTypes, &parameterType);
                }

                declaration->base.base.irType = LLVMFunctionType(_IRBuilderGetIRType(builder, declaration->returnType),
                                                                 (LLVMTypeRef *)ArrayGetMemoryPointer(temporaryTypes),
                                                                 (unsigned)ArrayGetElementCount(temporaryTypes), false);
            }

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)child;
                LLVMTypeRef structureType              = (LLVMTypeRef)declaration->base.base.irType;
                assert(structureType);

                for (Index index = 0; index < ASTArrayGetElementCount(declaration->values); index++) {
                    ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->values, index);
                    LLVMTypeRef valueType        = _IRBuilderGetIRType(builder, ASTNodeGetType(value));
                    assert(valueType);
                    value->base.base.irType = valueType;
                    ArrayAppendElement(temporaryTypes, &valueType);
                }
                LLVMStructSetBody(structureType, (LLVMTypeRef *)ArrayGetMemoryPointer(temporaryTypes), (unsigned)ArrayGetElementCount(temporaryTypes),
                                  false);

                ASTArrayIteratorRef iterator = ASTArrayGetIterator(declaration->initializers);
                while (iterator) {
                    ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)ASTArrayIteratorGetElement(iterator);
                    ArrayRemoveAllElements(temporaryTypes, true);

                    for (Index index = 0; index < ASTArrayGetElementCount(initializer->parameters); index++) {
                        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(initializer->parameters,
                                                                                                             index);
                        LLVMTypeRef parameterType        = _IRBuilderGetIRType(builder, ASTNodeGetType(parameter));
                        assert(parameterType);
                        parameter->base.base.irType = parameterType;
                        ArrayAppendElement(temporaryTypes, &parameterType);
                    }

                    initializer->base.base.irType = LLVMFunctionType(structureType, (LLVMTypeRef *)ArrayGetMemoryPointer(temporaryTypes),
                                                                     (unsigned)ArrayGetElementCount(temporaryTypes), false);
                    iterator                      = ASTArrayIteratorNext(iterator);
                }
            }

            if (child->tag == ASTTagValueDeclaration) {
                ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)child;
                declaration->base.base.irType      = _IRBuilderGetIRType(builder, ASTNodeGetType(declaration));
            }
        }
    }
    ArrayDestroy(temporaryTypes);
}

static inline void _IRBuilderBuildGlobalVariables(IRBuilderRef builder, ASTModuleDeclarationRef module) {
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagValueDeclaration) {
                _IRBuilderBuildGlobalVariable(builder, (ASTValueDeclarationRef)child);
            }
        }
    }
}

static inline void _IRBuilderBuildGlobalVariable(IRBuilderRef builder, ASTValueDeclarationRef declaration) {
    assert(!declaration->base.base.irValue);
    assert(declaration->base.base.irType);

    LLVMTypeRef type   = (LLVMTypeRef)declaration->base.base.irType;
    LLVMValueRef value = LLVMAddGlobal(builder->module->module, type, StringGetCharacters(declaration->base.mangledName));
    if (declaration->initializer) {
        // TODO: Check if initializer is constant, if so set initializer of global else emit initialization of value into program entry
        // point, this will also require the creation of a global value initialization dependency graphs which would track cyclic
        // initializations in global scope and also be helpful for topological sorting of initialization instructions
        if ((declaration->initializer->base.flags & ASTFlagsIsConstantEvaluable)) {
            _IRBuilderBuildConstantExpression(builder, declaration->initializer);
            LLVMSetInitializer(value, declaration->initializer->base.irValue);
        } else {
            ReportError("Expression is either not constant or it is currently not supported by the compiler!");
        }
    } else {
        LLVMValueRef initializer = LLVMConstNull(_IRBuilderGetIRType(builder, ASTNodeGetType(declaration)));
        LLVMSetInitializer(value, initializer);
    }

    declaration->base.base.irValue = value;
    declaration->base.base.flags |= ASTFlagsIsValuePointer;
}

static inline void _IRBuilderBuildEnumerationElements(IRBuilderRef builder, ASTEnumerationDeclarationRef declaration) {
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(declaration->elements);
    while (iterator) {
        ASTValueDeclarationRef value = ASTArrayIteratorGetElement(iterator);
        assert(value->initializer && value->base.mangledName);

        value->base.base.irType = _IRBuilderGetIRType(builder, ASTNodeGetType(value));
        _IRBuilderBuildGlobalVariable(builder, value);

        iterator = ASTArrayIteratorNext(iterator);
    }
}

static inline void _IRBuilderBuildFunctionSignature(IRBuilderRef builder, ASTFunctionDeclarationRef declaration) {
    if (declaration->base.base.irValue) {
        return;
    }

    assert(declaration->base.base.tag == ASTTagFunctionDeclaration);
    assert(declaration->base.base.irType);

    declaration->base.base.irValue = LLVMAddFunction(builder->module->module, StringGetCharacters(declaration->base.mangledName),
                                                     (LLVMTypeRef)declaration->base.base.irType);
}

static inline void _IRBuilderBuildFunctionBody(IRBuilderRef builder, ASTFunctionDeclarationRef declaration) {
    // TODO: Check if function has already been build and assert if so
    assert(declaration->base.base.tag == ASTTagFunctionDeclaration);
    assert(declaration->base.base.irType);
    assert(declaration->base.base.irValue);

    LLVMValueRef function = (LLVMValueRef)declaration->base.base.irValue;

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        parameter->base.base.irValue     = LLVMGetParam(function, index);
    }

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilder(builder->builder, entry, NULL);

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->body->statements); index++) {
        ASTNodeRef statement = (ASTNodeRef)ASTArrayGetElementAtIndex(declaration->body->statements, index);
        _IRBuilderBuildStatement(builder, function, statement);

        if (statement->flags & ASTFlagsStatementIsAlwaysReturning) {
            LLVMBuildUnreachable(builder->builder);
            return;
        }
    }

    if (!(declaration->body->base.flags & ASTFlagsStatementIsAlwaysReturning)) {
        LLVMBuildRetVoid(builder->builder);
    }
}

static inline void _IRBuilderBuildForeignFunctionSignature(IRBuilderRef builder, ASTFunctionDeclarationRef declaration) {
    if (declaration->base.base.irValue) {
        return;
    }

    assert(declaration->base.base.tag == ASTTagForeignFunctionDeclaration);

    if (declaration->base.base.irType == NULL) {
        declaration->base.base.irType = _IRBuilderGetIRType(builder, ASTNodeGetType(declaration));
    }

    assert(declaration->base.base.irType);

    LLVMValueRef function          = LLVMAddFunction(builder->module->module, StringGetCharacters(declaration->foreignName),
                                            (LLVMTypeRef)declaration->base.base.irType);
    declaration->base.base.irValue = function;

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        parameter->base.base.irValue     = LLVMGetParam(function, index);
    }

    // We are using the C calling convention for all foreign function declarations for now because we do not allow to specify the
    // calling convention in the AST
    LLVMSetFunctionCallConv(function, LLVMCCallConv);
}

static inline void _IRBuilderBuildInitializerSignature(IRBuilderRef builder, ASTInitializerDeclarationRef declaration) {
    if (declaration->base.base.irValue) {
        return;
    }

    assert(declaration->base.base.irType);

    declaration->base.base.irValue = LLVMAddFunction(builder->module->module, StringGetCharacters(declaration->base.mangledName),
                                                     (LLVMTypeRef)declaration->base.base.irType);
}

static inline void _IRBuilderBuildInitializerBody(IRBuilderRef builder, ASTStructureDeclarationRef structure,
                                                  ASTInitializerDeclarationRef declaration) {
    LLVMValueRef function = (LLVMValueRef)declaration->base.base.irValue;
    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        parameter->base.base.irValue     = LLVMGetParam(function, index);
    }

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilder(builder->builder, entry, NULL);

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->body->statements); index++) {
        ASTNodeRef statement = (ASTNodeRef)ASTArrayGetElementAtIndex(declaration->body->statements, index);
        _IRBuilderBuildStatement(builder, function, statement);

        if (statement->flags & ASTFlagsStatementIsAlwaysReturning) {
            LLVMBuildUnreachable(builder->builder);
            return;
        }
    }

    if (!(declaration->body->base.flags & ASTFlagsStatementIsAlwaysReturning)) {
        assert(declaration->implicitSelf->base.base.irValue);

        LLVMValueRef Value = _IRBuilderBuildLoad(builder, (ASTNodeRef)declaration->implicitSelf, "");
        LLVMBuildRet(builder->builder, Value);
    }
}

static inline void _IRBuilderBuildLocalVariable(IRBuilderRef builder, LLVMValueRef function, ASTValueDeclarationRef declaration) {
    // For now we will always alloca every local variable because we are not tracking in earlier passes if the value is referenced inside
    // the scope performing an alloca by default will guarantee some grade of correctness and could still be optimized by llvm passes
    assert(!declaration->base.base.irValue);
    assert(!declaration->base.mangledName);

    declaration->base.base.irType = _IRBuilderGetIRType(builder, ASTNodeGetType(declaration));

    if (declaration->initializer) {
        _IRBuilderBuildExpression(builder, function, declaration->initializer);
    }

    LLVMTypeRef type   = (LLVMTypeRef)declaration->base.base.irType;
    LLVMValueRef value = LLVMBuildAlloca(builder->builder, type, StringGetCharacters(declaration->base.name));
    if (declaration->initializer) {
        LLVMBuildStore(builder->builder, _IRBuilderLoadExpression(builder, function, declaration->initializer), value);
    }

    declaration->base.base.irValue = value;
    declaration->base.base.flags |= ASTFlagsIsValuePointer;
}

static inline void _IRBuilderBuildBlock(IRBuilderRef builder, LLVMValueRef function, ASTBlockRef block) {
    for (Index index = 0; index < ASTArrayGetElementCount(block->statements); index++) {
        ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(block->statements, index);
        _IRBuilderBuildStatement(builder, function, child);
        if (child->flags & ASTFlagsBlockHasTerminator) {
            break;
        }
    }
}

static inline void _IRBuilderBuildStatement(IRBuilderRef builder, LLVMValueRef function, ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagIfStatement:
        return _IRBuilderBuildIfStatement(builder, function, (ASTIfStatementRef)node);

    case ASTTagLoopStatement:
        return _IRBuilderBuildLoopStatement(builder, function, (ASTLoopStatementRef)node);

    case ASTTagSwitchStatement:
        return _IRBuilderBuildSwitchStatement(builder, function, (ASTSwitchStatementRef)node);

    case ASTTagControlStatement:
        return _IRBuilderBuildControlStatement(builder, function, (ASTControlStatementRef)node);

    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagIdentifierExpression:
    case ASTTagMemberAccessExpression:
    case ASTTagAssignmentExpression:
    case ASTTagCallExpression:
    case ASTTagConstantExpression:
        return _IRBuilderBuildExpression(builder, function, (ASTExpressionRef)node);

    case ASTTagValueDeclaration:
        return _IRBuilderBuildLocalVariable(builder, function, (ASTValueDeclarationRef)node);

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTStatement!");
        break;
    }
}

static inline void _IRBuilderBuildIfStatement(IRBuilderRef builder, LLVMValueRef function, ASTIfStatementRef statement) {
    LLVMBasicBlockRef entryBB  = LLVMGetInsertBlock(builder->builder);
    LLVMBasicBlockRef branchBB = LLVMAppendBasicBlock(function, "if-branch");
    LLVMBasicBlockRef thenBB   = LLVMAppendBasicBlock(function, "if-then");
    LLVMBasicBlockRef elseBB   = LLVMAppendBasicBlock(function, "if-else");
    LLVMBasicBlockRef mergeBB  = LLVMAppendBasicBlock(function, "if-merge");

    LLVMPositionBuilder(builder->builder, entryBB, NULL);
    LLVMBuildBr(builder->builder, branchBB);

    LLVMPositionBuilder(builder->builder, branchBB, NULL);
    _IRBuilderBuildExpression(builder, function, statement->condition);
    LLVMBuildCondBr(builder->builder, _IRBuilderLoadExpression(builder, function, statement->condition), thenBB, elseBB);

    LLVMPositionBuilder(builder->builder, thenBB, NULL);
    _IRBuilderBuildBlock(builder, function, statement->thenBlock);
    if (!(statement->thenBlock->base.flags & ASTFlagsBlockHasTerminator)) {
        LLVMBuildBr(builder->builder, mergeBB);
    }

    LLVMPositionBuilder(builder->builder, elseBB, NULL);
    _IRBuilderBuildBlock(builder, function, statement->elseBlock);
    if (!(statement->elseBlock->base.flags & ASTFlagsBlockHasTerminator)) {
        LLVMBuildBr(builder->builder, mergeBB);
    }

    LLVMPositionBuilder(builder->builder, mergeBB, NULL);
}

static inline void _IRBuilderBuildLoopStatement(IRBuilderRef builder, LLVMValueRef function, ASTLoopStatementRef statement) {
    LLVMBasicBlockRef entryBB  = LLVMGetInsertBlock(builder->builder);
    LLVMBasicBlockRef branchBB = LLVMAppendBasicBlock(function, "loop-branch");
    LLVMBasicBlockRef bodyBB   = LLVMAppendBasicBlock(function, "loop-body");
    LLVMBasicBlockRef endBB    = LLVMAppendBasicBlock(function, "loop-end");

    statement->irEntry = bodyBB;
    statement->irExit  = endBB;

    LLVMBasicBlockRef startBB = NULL;
    switch (statement->kind) {
    case ASTLoopKindWhile: {
        startBB = branchBB;
        break;
    }

    case ASTLoopKindDo: {
        startBB = bodyBB;
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid kind given for ASTLoopStatement");
        break;
    }

    LLVMPositionBuilder(builder->builder, entryBB, NULL);
    LLVMBuildBr(builder->builder, startBB);

    LLVMPositionBuilder(builder->builder, branchBB, NULL);
    _IRBuilderBuildExpression(builder, function, statement->condition);
    LLVMBuildCondBr(builder->builder, _IRBuilderLoadExpression(builder, function, statement->condition), bodyBB, endBB);

    LLVMPositionBuilder(builder->builder, bodyBB, NULL);
    _IRBuilderBuildBlock(builder, function, statement->loopBlock);
    if (!(statement->loopBlock->base.flags & ASTFlagsBlockHasTerminator)) {
        LLVMBuildBr(builder->builder, branchBB);
    }

    LLVMPositionBuilder(builder->builder, endBB, NULL);
}

static inline void _IRBuilderBuildSwitchStatement(IRBuilderRef builder, LLVMValueRef function, ASTSwitchStatementRef statement) {
    LLVMBasicBlockRef insertBB = LLVMGetInsertBlock(builder->builder);
    LLVMBasicBlockRef branchBB = LLVMAppendBasicBlock(function, "switch-branch");
    LLVMBasicBlockRef endBB    = LLVMAppendBasicBlock(function, "switch-end");

    statement->irExit = endBB;

    LLVMPositionBuilder(builder->builder, insertBB, NULL);
    LLVMBuildBr(builder->builder, branchBB);

    LLVMPositionBuilder(builder->builder, branchBB, NULL);
    _IRBuilderBuildExpression(builder, function, statement->argument);

    assert(ASTArrayGetElementCount(statement->cases) > 0);

    LLVMBasicBlockRef caseBodyBB = LLVMAppendBasicBlock(function, "switch-body");

    ASTArrayIteratorRef iterator = ASTArrayGetIterator(statement->cases);
    while (iterator) {
        ASTArrayIteratorRef iteratorNext = ASTArrayIteratorNext(iterator);

        LLVMBasicBlockRef caseBranchBB = LLVMGetInsertBlock(builder->builder);
        ASTCaseStatementRef child      = (ASTCaseStatementRef)ASTArrayIteratorGetElement(iterator);

        LLVMPositionBuilder(builder->builder, caseBranchBB, NULL);

        LLVMBasicBlockRef nextBodyBB   = NULL;
        LLVMBasicBlockRef nextBranchBB = endBB;
        if (iteratorNext) {
            nextBodyBB    = LLVMAppendBasicBlock(function, "switch-body");
            nextBranchBB  = LLVMAppendBasicBlock(function, "switch-branch");
            child->irNext = nextBodyBB;
        }

        if (child->kind == ASTCaseKindConditional) {
            assert(child->comparator);

            _IRBuilderBuildExpression(builder, function, child->condition);
            LLVMValueRef arguments[] = {_IRBuilderLoadExpression(builder, function, statement->argument),
                                        _IRBuilderLoadExpression(builder, function, child->condition)};
            LLVMValueRef condition   = _IRBuilderBuildBinaryExpression(builder, function, child->comparator, arguments);
            LLVMBuildCondBr(builder->builder, condition, caseBodyBB, nextBranchBB);
        } else {
            LLVMBuildBr(builder->builder, caseBodyBB);
        }

        LLVMPositionBuilder(builder->builder, caseBodyBB, NULL);
        ASTArrayIteratorRef bodyIterator = ASTArrayGetIterator(child->body->statements);
        while (bodyIterator) {
            ASTNodeRef bodyChild = (ASTNodeRef)ASTArrayIteratorGetElement(bodyIterator);
            _IRBuilderBuildStatement(builder, function, bodyChild);

            bodyIterator = ASTArrayIteratorNext(bodyIterator);
        }

        if (!(child->body->base.flags & ASTFlagsBlockHasTerminator)) {
            LLVMBuildBr(builder->builder, endBB);
        }

        caseBodyBB = nextBodyBB;
        iterator   = iteratorNext;
        LLVMPositionBuilder(builder->builder, nextBranchBB, NULL);
    }

    LLVMPositionBuilder(builder->builder, endBB, NULL);
}

static inline void _IRBuilderBuildControlStatement(IRBuilderRef builder, LLVMValueRef function, ASTControlStatementRef statement) {
    switch (statement->kind) {
    case ASTControlKindBreak: {
        assert(statement->enclosingNode);
        if (statement->enclosingNode->tag == ASTTagSwitchStatement) {
            ASTSwitchStatementRef parent = (ASTSwitchStatementRef)statement->enclosingNode;
            assert(parent->irExit);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)parent->irExit);
            return;
        } else if (statement->enclosingNode->tag == ASTTagLoopStatement) {
            ASTLoopStatementRef parent = (ASTLoopStatementRef)statement->enclosingNode;
            assert(parent->irExit);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)parent->irExit);
            return;
        }
        break;
    }

    case ASTControlKindContinue: {
        assert(statement->enclosingNode);
        if (statement->enclosingNode->tag == ASTTagLoopStatement) {
            ASTLoopStatementRef loop = (ASTLoopStatementRef)statement->enclosingNode;
            assert(loop->irEntry);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)loop->irEntry);
            return;
        }
        break;
    }

    case ASTControlKindFallthrough: {
        assert(statement->enclosingNode);
        if (statement->enclosingNode->tag == ASTTagCaseStatement) {
            ASTCaseStatementRef caseStatement = (ASTCaseStatementRef)statement->enclosingNode;
            assert(caseStatement->irNext);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)caseStatement->irNext);
            return;
        }
        break;
    }

    case ASTControlKindReturn: {
        if (statement->result) {
            _IRBuilderBuildExpression(builder, function, statement->result);
            if (ASTNodeGetType(statement->result)->tag == ASTTagBuiltinType &&
                ((ASTBuiltinTypeRef)ASTNodeGetType(statement->result))->kind == ASTBuiltinTypeKindVoid) {
                LLVMBuildRetVoid(builder->builder);
                return;
            }

            assert(statement->enclosingNode && statement->enclosingNode->tag == ASTTagFunctionDeclaration);
            ASTFunctionDeclarationRef func = (ASTFunctionDeclarationRef)statement->enclosingNode;
            ASTTypeRef targetType          = func->returnType;
            LLVMValueRef resultValue       = _IRBuilderLoadExpression(builder, function, statement->result);
            resultValue = _IRBuilderImplicitlyConvertValue(builder, function, resultValue, statement->result, targetType);
            LLVMBuildRet(builder->builder, resultValue);
            return;
        } else {
            LLVMBuildRetVoid(builder->builder);
            return;
        }
        break;
    }

    default:
        break;
    }

    ReportCritical("Internal compiler error!");
}

static inline void _IRBuilderBuildExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression) {
    switch (expression->base.tag) {
    case ASTTagReferenceExpression: {
        ASTReferenceExpressionRef reference = (ASTReferenceExpressionRef)expression;
        _IRBuilderBuildExpression(builder, function, reference->argument);
        LLVMValueRef pointer = NULL;
        if (reference->argument->base.flags & ASTFlagsIsValuePointer) {
            pointer = reference->argument->base.irValue;
        } else {
            pointer = LLVMBuildAlloca(builder->builder, (LLVMTypeRef)reference->argument->base.irType, "");
            LLVMBuildStore(builder->builder, _IRBuilderLoadExpression(builder, function, reference->argument), pointer);
        }

        reference->base.base.irType  = _IRBuilderGetIRType(builder, ASTNodeGetType(reference));
        reference->base.base.irValue = pointer;
        return;
    }

    case ASTTagDereferenceExpression: {
        ASTDereferenceExpressionRef dereference = (ASTDereferenceExpressionRef)expression;
        _IRBuilderBuildExpression(builder, function, dereference->argument);
        LLVMTypeRef Type = _IRBuilderGetIRType(builder, ASTNodeGetType(dereference->argument));
        dereference->base.base.irType = _IRBuilderGetIRType(builder, ASTNodeGetType(dereference));
        dereference->base.base.irValue = LLVMBuildLoad2(builder->builder, Type, (LLVMValueRef)dereference->argument->base.irValue, "");
        return;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)expression;
        assert(identifier->resolvedDeclaration && identifier->resolvedDeclaration->base.irValue);
        if (identifier->resolvedDeclaration->base.flags & ASTFlagsIsValuePointer) {
            identifier->base.base.flags |= ASTFlagsIsValuePointer;
        }

        identifier->base.base.irType  = identifier->resolvedDeclaration->base.irType;
        identifier->base.base.irValue = identifier->resolvedDeclaration->base.irValue;

        if (identifier->resolvedDeclaration->base.tag == ASTTagFunctionDeclaration) {
            identifier->base.base.irType = LLVMPointerType((LLVMTypeRef)identifier->base.base.irType, 0);
            identifier->base.base.flags |= ASTFlagsIsValuePointer;
        }

        return;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef MemberAccess = (ASTMemberAccessExpressionRef)expression;
        assert(MemberAccess->memberIndex >= 0);

        _IRBuilderBuildExpression(builder, function, MemberAccess->argument);

        ASTTypeRef ArgumentType = ASTNodeGetType(MemberAccess->argument);
        LLVMValueRef Pointer = NULL;
        if (MemberAccess->argument->base.flags & ASTFlagsIsValuePointer) {
            Pointer = MemberAccess->argument->base.irValue;
        } else {
            Pointer = LLVMBuildAlloca(builder->builder, _IRBuilderGetIRType(builder, ArgumentType), "");
            LLVMBuildStore(builder->builder, _IRBuilderLoadExpression(builder, function, MemberAccess->argument), Pointer);
        }

        Int PointerDepth = MemberAccess->pointerDepth;
        while (PointerDepth > 0) {
            Pointer = LLVMBuildLoad2(builder->builder, _IRBuilderGetIRType(builder, ArgumentType), Pointer, "");
            PointerDepth -= 1;
            
            assert(ArgumentType->tag == ASTTagPointerType);
            ASTPointerTypeRef PointerType = (ASTPointerTypeRef)ArgumentType;
            ArgumentType = PointerType->pointeeType;
        }
        
        LLVMTypeRef PointerType = _IRBuilderGetIRType(builder, ArgumentType);
        MemberAccess->base.base.irType  = MemberAccess->argument->base.irType;
        MemberAccess->base.base.irValue = LLVMBuildStructGEP2(builder->builder, PointerType, Pointer, (unsigned)MemberAccess->memberIndex, "");
        MemberAccess->base.base.flags |= ASTFlagsIsValuePointer;
        return;
    }

    case ASTTagAssignmentExpression: {
        ASTAssignmentExpressionRef assignment = (ASTAssignmentExpressionRef)expression;
        assert(assignment->op == ASTBinaryOperatorAssign && "Composite assignment operations are not supported yet!");
        _IRBuilderBuildExpression(builder, function, assignment->variable);
        _IRBuilderBuildExpression(builder, function, assignment->expression);
        assignment->base.base.irType  = LLVMVoidType();
        assignment->base.base.irValue = LLVMBuildStore(builder->builder,
                                                       _IRBuilderLoadExpression(builder, function, assignment->expression),
                                                       (LLVMValueRef)assignment->variable->base.irValue);
        return;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)expression;
        assert(ASTNodeGetType(call->callee)->tag == ASTTagFunctionType ||
               (ASTNodeGetType(call->callee)->tag == ASTTagPointerType &&
                ((ASTPointerTypeRef)ASTNodeGetType(call->callee))->pointeeType->tag == ASTTagFunctionType));

        // Prefix and infix functions are currently not added to the declarations of the module and are just contained inside the global
        // scope, so we will force the IR generation of the function here for now...
        // TODO: Remove this after finishing implementation for foreign and prefix infix functions
        if (!ASTNodeGetType(call->callee)->irType) {
            _IRBuilderGetIRType(builder, ASTNodeGetType(call->callee));
        }

        if (call->base.base.flags & ASTFlagsCallIsInitialization) {
            assert(call->callee->base.tag == ASTTagIdentifierExpression);
            ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)call->callee;
            assert(identifier->resolvedDeclaration && identifier->resolvedDeclaration->base.tag == ASTTagInitializerDeclaration);
            ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)identifier->resolvedDeclaration;

            ArrayRef arguments = ArrayCreateEmpty(builder->allocator, sizeof(LLVMValueRef), ASTArrayGetElementCount(call->arguments));
            for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
                ASTExpressionRef argument        = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
                ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(initializer->parameters, index);
                _IRBuilderBuildExpression(builder, function, argument);
                LLVMValueRef argumentValue = _IRBuilderLoadExpression(builder, function, argument);
                argumentValue = _IRBuilderImplicitlyConvertValue(builder, function, argumentValue, argument, ASTNodeGetType(parameter));
                ArrayAppendElement(arguments, &argumentValue);
            }

            LLVMTypeRef Type = _IRBuilderGetIRType(builder, ASTNodeGetType(initializer));
            call->base.base.irValue = (LLVMValueRef)LLVMBuildCall2(builder->builder,
                                                                   Type,
                                                                   (LLVMValueRef)initializer->base.base.irValue,
                                                                   (LLVMValueRef *)ArrayGetMemoryPointer(arguments),
                                                                   (unsigned)ArrayGetElementCount(arguments), "");

            ArrayDestroy(arguments);
            return;
        }

        if (call->base.base.flags & ASTFlagsIsPointerArithmetic) {
            assert(ASTArrayGetElementCount(call->arguments) == 2);
            ASTExpressionRef arguments[] = {ASTArrayGetElementAtIndex(call->arguments, 0), ASTArrayGetElementAtIndex(call->arguments, 1)};
            _IRBuilderBuildExpression(builder, function, arguments[0]);
            _IRBuilderBuildExpression(builder, function, arguments[1]);
            LLVMValueRef pointer    = _IRBuilderLoadExpression(builder, function, arguments[0]);
            LLVMValueRef indices[]  = {_IRBuilderLoadExpression(builder, function, arguments[1])};
            call->base.base.irValue = LLVMBuildGEP2(builder->builder, LLVMTypeOf(pointer), pointer, indices, 1, "");
            return;
        }

        ASTFunctionDeclarationRef declaration = NULL;
        ASTFunctionTypeRef functionType       = NULL;
        if (ASTNodeGetType(call->callee)->tag == ASTTagPointerType) {
            ASTPointerTypeRef pointerType = (ASTPointerTypeRef)ASTNodeGetType(call->callee);
            functionType                  = (ASTFunctionTypeRef)pointerType->pointeeType;
            assert(functionType->resultType->irType);
            call->base.base.irType = functionType->resultType->irType;
            declaration            = functionType->declaration;
        } else {
            functionType = (ASTFunctionTypeRef)ASTNodeGetType(call->callee);
            assert(functionType->resultType->irType);
            call->base.base.irType = functionType->resultType->irType;
            declaration            = functionType->declaration;
        }

        if (!declaration || declaration->base.base.tag == ASTTagFunctionDeclaration) {
            _IRBuilderBuildExpression(builder, function, call->callee);

            ArrayRef arguments = ArrayCreateEmpty(builder->allocator, sizeof(LLVMValueRef), ASTArrayGetElementCount(call->arguments));
            for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
                ASTExpressionRef argument = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
                ASTTypeRef parameterType  = ASTArrayGetElementAtIndex(functionType->parameterTypes, index);
                _IRBuilderBuildExpression(builder, function, argument);
                LLVMValueRef argumentValue = _IRBuilderLoadExpression(builder, function, argument);
                argumentValue              = _IRBuilderImplicitlyConvertValue(builder, function, argumentValue, argument, parameterType);
                ArrayAppendElement(arguments, &argumentValue);
            }

            LLVMTypeRef Type = _IRBuilderGetIRType(builder, ASTNodeGetType(call->callee));
            LLVMValueRef Callee = call->callee->base.irValue;
            if (ASTNodeGetType(call->callee)->tag == ASTTagPointerType) {
                ASTPointerTypeRef PointerType = (ASTPointerTypeRef)ASTNodeGetType(call->callee);
                Type = _IRBuilderGetIRType(builder, PointerType->pointeeType);
                Callee = _IRBuilderLoadExpression(builder, function, call->callee);
            }

            LLVMValueRef *Args = (LLVMValueRef *)ArrayGetMemoryPointer(arguments);
            unsigned NumArgs = (unsigned)ArrayGetElementCount(arguments);
            
            call->base.base.irValue = (LLVMValueRef)LLVMBuildCall2(builder->builder, Type, Callee, Args, NumArgs, "");
            ArrayDestroy(arguments);
        } else if (declaration->base.base.tag == ASTTagForeignFunctionDeclaration) {
            _IRBuilderBuildForeignFunctionSignature(builder, declaration);

            ArrayRef arguments = ArrayCreateEmpty(builder->allocator, sizeof(LLVMValueRef), ASTArrayGetElementCount(call->arguments));
            for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
                ASTExpressionRef argument        = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
                ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
                _IRBuilderBuildExpression(builder, function, argument);
                LLVMValueRef argumentValue = _IRBuilderLoadExpression(builder, function, argument);
                argumentValue = _IRBuilderImplicitlyConvertValue(builder, function, argumentValue, argument, ASTNodeGetType(parameter));
                ArrayAppendElement(arguments, &argumentValue);
            }

            LLVMTypeRef Type = _IRBuilderGetIRType(builder, ASTNodeGetType(declaration));
            call->base.base.irValue = LLVMBuildCall2(builder->builder,
                                                     Type,
                                                     (LLVMValueRef)declaration->base.base.irValue,
                                                     (LLVMValueRef *)ArrayGetMemoryPointer(arguments),
                                                     (unsigned)ArrayGetElementCount(arguments), "");
            ArrayDestroy(arguments);
        } else if (declaration->base.base.tag == ASTTagIntrinsicFunctionDeclaration) {
            ArrayRef arguments = ArrayCreateEmpty(builder->allocator, sizeof(LLVMValueRef), ASTArrayGetElementCount(call->arguments));
            for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
                ASTExpressionRef argument        = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
                ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
                _IRBuilderBuildExpression(builder, function, argument);
                argument->base.irValue = _IRBuilderLoadExpression(builder, function, argument);
                argument->base.irValue = _IRBuilderImplicitlyConvertValue(builder, function, argument->base.irValue, argument,
                                                                          ASTNodeGetType(parameter));
                ArrayAppendElement(arguments, &argument->base.irValue);
            }

            call->base.base.irValue = _IRBuilderBuildIntrinsic(
                builder, function, declaration->intrinsicName, (LLVMValueRef *)ArrayGetMemoryPointer(arguments),
                (unsigned)ArrayGetElementCount(arguments), (LLVMTypeRef)declaration->returnType->irType);
            ArrayDestroy(arguments);
        } else {
            JELLY_UNREACHABLE("Invalid tag given for ASTFunctionDeclaration!");
        }

        return;
    }

    case ASTTagConstantExpression:
        return _IRBuilderBuildConstantExpression(builder, expression);

    case ASTTagSizeOfExpression: {
        ASTSizeOfExpressionRef sizeOf = (ASTSizeOfExpressionRef)expression;
        sizeOf->base.base.irValue     = _IRBuilderGetConstantSizeOfType(builder, sizeOf->sizeType);
        return;
    }

    case ASTTagSubscriptExpression: {
        ASTSubscriptExpressionRef Subscript = (ASTSubscriptExpressionRef)expression;
        // NOTE: Subscript expressions are currently only allowed for static array types and are handled in the semantic phase
        //       so we assume it only contains a single argument which is a constant of integer type...
        assert(ASTArrayGetElementCount(Subscript->arguments) == 1);

        ASTExpressionRef Argument = ASTArrayGetElementAtIndex(Subscript->arguments, 0);
        assert(ASTTypeIsInteger(ASTNodeGetType(Argument)));

        _IRBuilderBuildExpression(builder, function, Subscript->expression);
        _IRBuilderBuildExpression(builder, function, Argument);

        LLVMTypeRef Type = Subscript->expression->base.irType;
        LLVMValueRef Pointer = Subscript->expression->base.irValue;
        LLVMValueRef Indices[] = {
            LLVMConstInt((LLVMTypeRef)ASTNodeGetType(Argument)->irType, 0, false),
            _IRBuilderLoadExpression(builder, function, Argument)
        };
        Subscript->base.base.irValue = LLVMBuildInBoundsGEP2(builder->builder, Type, Pointer, Indices, 2, "");
        Subscript->base.base.flags |= ASTFlagsIsValuePointer;
        return;
    }

    case ASTTagTypeOperationExpression: {
        ASTTypeOperationExpressionRef typeExpression = (ASTTypeOperationExpressionRef)expression;
        _IRBuilderBuildExpression(builder, function, typeExpression->expression);

        switch (typeExpression->op) {
        case ASTTypeOperationTypeCheck:
            ReportCritical("Type check operation is currently not supported!");
            return;

        case ASTTypeOperationTypeCast:
            ReportCritical("Type cast operation is currently not supported!");
            return;

        case ASTTypeOperationTypeBitcast: {
            LLVMValueRef value                = _IRBuilderLoadExpression(builder, function, typeExpression->expression);
            LLVMTypeRef targetType            = _IRBuilderGetIRType(builder, typeExpression->argumentType);
            typeExpression->base.base.irValue = LLVMBuildBitCast(builder->builder, value, targetType, "");
            return;
        }
        }

        return;
    }

    default:
        break;
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTExpression!");
}

static inline void _IRBuilderBuildConstantExpression(IRBuilderRef builder, ASTExpressionRef expression) {
    LLVMTypeRef type = _IRBuilderGetIRType(builder, ASTNodeGetType(expression));
    if (expression->base.tag == ASTTagConstantExpression) {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)expression;
        switch (constant->kind) {
        case ASTConstantKindNil:
            constant->base.base.irValue = LLVMConstNull(type);
            return;

        case ASTConstantKindBool:
            constant->base.base.irValue = LLVMConstInt(type, constant->boolValue ? 1 : 0, false);
            return;

        case ASTConstantKindInt:
            constant->base.base.irValue = LLVMConstInt(type, constant->intValue, false);
            return;

        case ASTConstantKindFloat:
            constant->base.base.irValue = LLVMConstReal(type, constant->floatValue);
            return;

        case ASTConstantKindString: {
            UInt32 StringLength     = (UInt32)StringGetLength(constant->stringValue);
            Char* StringValue       = StringGetCharacters(constant->stringValue);
            LLVMValueRef BufferData = LLVMConstStringInContext(builder->context, StringValue, StringLength, false);
            LLVMValueRef BufferVar  = LLVMAddGlobal(builder->module->module, LLVMArrayType(LLVMInt8Type(), StringLength + 1), "");
            LLVMSetGlobalConstant(BufferVar, true);
            LLVMSetInitializer(BufferVar, BufferData);

            LLVMTypeRef StringType      = _IRBuilderGetIRType(builder, (ASTTypeRef)ASTContextGetStringType(builder->astContext));
            LLVMValueRef Indices[]      = {LLVMConstInt(LLVMInt32Type(), 0, false), LLVMConstInt(LLVMInt32Type(), 0, false)};
            LLVMValueRef BufferPtr      = LLVMConstGEP2(StringType, BufferVar, Indices, 2);
            LLVMValueRef StringValues[] = {BufferPtr, LLVMConstInt(LLVMInt64Type(), StringGetLength(constant->stringValue), true)};
            LLVMValueRef Initializer    = LLVMConstNamedStruct(StringType, StringValues, 2);
            constant->base.base.irValue = Initializer;
            return;
        }

        default:
            JELLY_UNREACHABLE("Invalid kind given for ASTConstantExpression!");
            return;
        }
    }

    JELLY_UNREACHABLE("Unsupported constant expression!");
}

static inline LLVMValueRef _IRBuilderBuildBinaryExpression(IRBuilderRef builder, LLVMValueRef function, ASTFunctionDeclarationRef callee,
                                                           LLVMValueRef *arguments) {
    // Infix functions are currently not added to the declarations of the module and are just contained inside the global scope, so we
    // will force the IR generation of the function here for now...
    if (!callee->base.base.irType) {
        assert(ASTArrayGetElementCount(callee->parameters) == 2);
        ASTValueDeclarationRef lhsParameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(callee->parameters, 0);
        ASTValueDeclarationRef rhsParameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(callee->parameters, 1);
        LLVMTypeRef parameterTypes[]        = {
            _IRBuilderGetIRType(builder, ASTNodeGetType(lhsParameter)),
            _IRBuilderGetIRType(builder, ASTNodeGetType(rhsParameter))
        };
        callee->base.base.irType            = LLVMFunctionType(_IRBuilderGetIRType(builder, callee->returnType), parameterTypes, 2, false);
    }

    if (callee->base.base.tag == ASTTagFunctionDeclaration) {
        _IRBuilderBuildFunctionBody(builder, callee);
        LLVMValueRef opFunction = (LLVMValueRef)callee->base.base.irValue;
        LLVMTypeRef Type = _IRBuilderGetIRType(builder, ASTNodeGetType(callee));
        return LLVMBuildCall2(builder->builder, Type, opFunction, arguments, 2, "");
    } else if (callee->base.base.tag == ASTTagForeignFunctionDeclaration) {
        _IRBuilderBuildForeignFunctionSignature(builder, callee);
        LLVMValueRef opFunction = (LLVMValueRef)callee->base.base.irValue;
        LLVMTypeRef Type = _IRBuilderGetIRType(builder, ASTNodeGetType(callee));
        return LLVMBuildCall2(builder->builder, Type, opFunction, arguments, 2, "");
    } else if (callee->base.base.tag == ASTTagIntrinsicFunctionDeclaration) {
        return _IRBuilderBuildIntrinsic(builder, function, callee->intrinsicName, arguments, 2, (LLVMTypeRef)callee->returnType->irType);
    } else {
        JELLY_UNREACHABLE("Invalid tag given for ASTFunctionDeclaration!");
    }
}

static inline LLVMValueRef _IRBuilderLoadExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression) {
    assert(expression->base.irValue);

    if (expression->base.flags & ASTFlagsIsValuePointer) {
        LLVMTypeRef Type = _IRBuilderGetIRType(builder, ASTNodeGetType(expression));
        return LLVMBuildLoad2(builder->builder, Type, (LLVMValueRef)expression->base.irValue, "");
    }

    return (LLVMValueRef)expression->base.irValue;
}

static inline LLVMTypeRef _IRBuilderGetIRType(IRBuilderRef builder, ASTTypeRef type) {
    assert(type && type->tag != ASTTagOpaqueType);

    if (type->irType) {
        return (LLVMTypeRef)type->irType;
    }

    LLVMTypeRef llvmType = NULL;
    switch (type->tag) {
    case ASTTagPointerType: {
        ASTPointerTypeRef pointerType = (ASTPointerTypeRef)type;
        
        // We substitute Void* to UInt8* here as Void* is not a valid pointer type.
        if (ASTTypeIsVoid(pointerType->pointeeType)) {
            llvmType = LLVMInt8Type();
        } else {
            llvmType = _IRBuilderGetIRType(builder, pointerType->pointeeType);
        }
        
        llvmType = LLVMPointerType(llvmType, 0);
        break;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef arrayType = (ASTArrayTypeRef)type;
        if (arrayType->base.flags & ASTFlagsArrayTypeIsStatic) {
            llvmType = _IRBuilderGetIRType(builder, arrayType->elementType);
            llvmType = LLVMArrayType(llvmType, (unsigned)arrayType->sizeValue);
        } else {
            ReportCritical("Dynamic Array type is currently not supported!");
        }
        break;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)type;
        switch (builtinType->kind) {
        case ASTBuiltinTypeKindVoid:
            llvmType = LLVMVoidType();
            break;

        case ASTBuiltinTypeKindBool:
            llvmType = LLVMInt1Type();
            break;

        case ASTBuiltinTypeKindInt8:
        case ASTBuiltinTypeKindUInt8:
            llvmType = LLVMInt8Type();
            break;

        case ASTBuiltinTypeKindInt16:
        case ASTBuiltinTypeKindUInt16:
            llvmType = LLVMInt16Type();
            break;

        case ASTBuiltinTypeKindInt32:
        case ASTBuiltinTypeKindUInt32:
            llvmType = LLVMInt32Type();
            break;

        case ASTBuiltinTypeKindInt64:
        case ASTBuiltinTypeKindUInt64:
        case ASTBuiltinTypeKindInt:
        case ASTBuiltinTypeKindUInt:
            llvmType = LLVMInt64Type();
            break;

        case ASTBuiltinTypeKindFloat32:
            llvmType = LLVMFloatType();
            break;

        case ASTBuiltinTypeKindFloat64:
        case ASTBuiltinTypeKindFloat:
            llvmType = LLVMDoubleType();
            break;

        default:
            JELLY_UNREACHABLE("Invalid kind given for ASTBuiltinType!");
            break;
        }

        break;
    }

    case ASTTagEnumerationType: {
        // TODO: Replace enumeration type with the least required bit size int based on the represented values of the enumeration elements
        llvmType = LLVMInt64Type();
        break;
    }

    case ASTTagFunctionType: {
        ASTFunctionTypeRef functionType = (ASTFunctionTypeRef)type;
        ArrayRef parameterTypes         = ArrayCreateEmpty(builder->allocator, sizeof(LLVMTypeRef),
                                                   ASTArrayGetElementCount(functionType->parameterTypes));
        ASTArrayIteratorRef iterator    = ASTArrayGetIterator(functionType->parameterTypes);
        while (iterator) {
            ASTTypeRef parameterType = ASTArrayIteratorGetElement(iterator);
            parameterType->irType    = _IRBuilderGetIRType(builder, parameterType);
            ArrayAppendElement(parameterTypes, &parameterType->irType);
            iterator = ASTArrayIteratorNext(iterator);
        }

        functionType->resultType->irType = _IRBuilderGetIRType(builder, functionType->resultType);

        llvmType = LLVMFunctionType((LLVMTypeRef)functionType->resultType->irType, (LLVMTypeRef *)ArrayGetMemoryPointer(parameterTypes),
                                    (unsigned)ArrayGetElementCount(parameterTypes), false);
        ArrayDestroy(parameterTypes);
        break;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef structureType = (ASTStructureTypeRef)type;
        if (structureType->declaration->base.base.irType) {
            llvmType = (LLVMTypeRef)structureType->declaration->base.base.irType;
        } else {
            llvmType = LLVMStructCreateNamed(builder->context, StringGetCharacters(structureType->declaration->base.mangledName));

            ArrayRef temporaryTypes = ArrayCreateEmpty(builder->allocator, sizeof(LLVMTypeRef),
                                                       ASTArrayGetElementCount(structureType->declaration->values));
            for (Index index = 0; index < ASTArrayGetElementCount(structureType->declaration->values); index++) {
                ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(structureType->declaration->values, index);
                LLVMTypeRef valueType        = _IRBuilderGetIRType(builder, ASTNodeGetType(value));
                ArrayAppendElement(temporaryTypes, &valueType);
            }
            LLVMStructSetBody(llvmType, (LLVMTypeRef *)ArrayGetMemoryPointer(temporaryTypes), (unsigned)ArrayGetElementCount(temporaryTypes), false);
            ArrayDestroy(temporaryTypes);
        }
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTType!");
        break;
    }

    type->irType = llvmType;
    return llvmType;
}

static inline LLVMValueRef _IRBuilderBuildIntrinsic(IRBuilderRef builder, LLVMValueRef function, StringRef intrinsic,
                                                    LLVMValueRef *arguments, unsigned argumentCount, LLVMTypeRef resultType) {
    if (StringIsEqualToCString(intrinsic, "bitwise_neg_i1")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument of type 'Bool'", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildSelect(builder->builder, arguments[0], LLVMConstInt(LLVMInt1Type(), 0, false),
                               LLVMConstInt(LLVMInt1Type(), 1, false), "");
    }

    if (StringIsEqualToCString(intrinsic, "bitwise_neg_i8")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument of type 'Int8' or 'UInt8'", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        LLVMValueRef mask = LLVMConstInt(LLVMInt8Type(), 0xFF, false);
        return LLVMBuildSub(builder->builder, mask, arguments[0], "");
    }

    if (StringIsEqualToCString(intrinsic, "bitwise_neg_i16")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument of type 'Int16' or 'UInt16'", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        LLVMValueRef mask = LLVMConstInt(LLVMInt16Type(), 0xFFFF, false);
        return LLVMBuildSub(builder->builder, mask, arguments[0], "");
    }

    if (StringIsEqualToCString(intrinsic, "bitwise_neg_i32")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument of type 'Int32' or 'UInt32'", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        LLVMValueRef mask = LLVMConstInt(LLVMInt32Type(), 0xFFFFFFFF, false);
        return LLVMBuildSub(builder->builder, mask, arguments[0], "");
    }

    if (StringIsEqualToCString(intrinsic, "bitwise_neg_i64")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument of type 'Int64' or 'UInt64'", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        LLVMValueRef mask = LLVMConstInt(LLVMInt64Type(), 0xFFFFFFFFFFFFFFFF, false);
        return LLVMBuildSub(builder->builder, mask, arguments[0], "");
    }

    if (StringIsEqualToCString(intrinsic, "arg_val_0")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return arguments[0];
    }

    if (StringIsEqualToCString(intrinsic, "neg_i8") || StringIsEqualToCString(intrinsic, "neg_i16") ||
        StringIsEqualToCString(intrinsic, "neg_i32") || StringIsEqualToCString(intrinsic, "neg_i64")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildNeg(builder->builder, arguments[0], "");
    }

    if (StringIsEqualToCString(intrinsic, "neg_f32") || StringIsEqualToCString(intrinsic, "neg_f64")) {
        if (argumentCount != 1) {
            ReportErrorFormat("Intrinsic '%s' expects one argument", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFNeg(builder->builder, arguments[0], "");
    }

    if (StringIsEqualToCString(intrinsic, "shl_i8") || StringIsEqualToCString(intrinsic, "shl_i16") ||
        StringIsEqualToCString(intrinsic, "shl_i32") || StringIsEqualToCString(intrinsic, "shl_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expectes two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildShl(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "shr_i8") || StringIsEqualToCString(intrinsic, "shr_i16") ||
        StringIsEqualToCString(intrinsic, "shr_i32") || StringIsEqualToCString(intrinsic, "shr_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildLShr(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "mul_i8") || StringIsEqualToCString(intrinsic, "mul_i16") ||
        StringIsEqualToCString(intrinsic, "mul_i32") || StringIsEqualToCString(intrinsic, "mul_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildMul(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "mul_f32") || StringIsEqualToCString(intrinsic, "mul_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFMul(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "div_s8") || StringIsEqualToCString(intrinsic, "div_s16") ||
        StringIsEqualToCString(intrinsic, "div_s32") || StringIsEqualToCString(intrinsic, "div_s64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildSDiv(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "div_u8") || StringIsEqualToCString(intrinsic, "div_u16") ||
        StringIsEqualToCString(intrinsic, "div_u32") || StringIsEqualToCString(intrinsic, "div_u64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildUDiv(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "div_f32") || StringIsEqualToCString(intrinsic, "div_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFDiv(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "rem_s8") || StringIsEqualToCString(intrinsic, "rem_s16") ||
        StringIsEqualToCString(intrinsic, "rem_s32") || StringIsEqualToCString(intrinsic, "rem_s64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildSRem(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "rem_u8") || StringIsEqualToCString(intrinsic, "rem_u16") ||
        StringIsEqualToCString(intrinsic, "rem_u32") || StringIsEqualToCString(intrinsic, "rem_u64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildURem(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "rem_f32") || StringIsEqualToCString(intrinsic, "rem_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFRem(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "bitwise_and_i1") || StringIsEqualToCString(intrinsic, "bitwise_and_i8") ||
        StringIsEqualToCString(intrinsic, "bitwise_and_i16") || StringIsEqualToCString(intrinsic, "bitwise_and_i32") ||
        StringIsEqualToCString(intrinsic, "bitwise_and_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildAnd(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "add_i8") || StringIsEqualToCString(intrinsic, "add_i16") ||
        StringIsEqualToCString(intrinsic, "add_i32") || StringIsEqualToCString(intrinsic, "add_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildAdd(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "add_f32") || StringIsEqualToCString(intrinsic, "add_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFAdd(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "sub_i8") || StringIsEqualToCString(intrinsic, "sub_i16") ||
        StringIsEqualToCString(intrinsic, "sub_i32") || StringIsEqualToCString(intrinsic, "sub_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildSub(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "sub_f32") || StringIsEqualToCString(intrinsic, "sub_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFSub(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "bitwise_or_i1") || StringIsEqualToCString(intrinsic, "bitwise_or_i8") ||
        StringIsEqualToCString(intrinsic, "bitwise_or_i16") || StringIsEqualToCString(intrinsic, "bitwise_or_i32") ||
        StringIsEqualToCString(intrinsic, "bitwise_or_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildOr(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "bitwise_xor_i8") || StringIsEqualToCString(intrinsic, "bitwise_xor_i16") ||
        StringIsEqualToCString(intrinsic, "bitwise_xor_i32") || StringIsEqualToCString(intrinsic, "bitwise_xor_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildXor(builder->builder, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_lt_s8") || StringIsEqualToCString(intrinsic, "cmp_lt_s16") ||
        StringIsEqualToCString(intrinsic, "cmp_lt_s32") || StringIsEqualToCString(intrinsic, "cmp_lt_s64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntSLT, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_lt_u8") || StringIsEqualToCString(intrinsic, "cmp_lt_u16") ||
        StringIsEqualToCString(intrinsic, "cmp_lt_u32") || StringIsEqualToCString(intrinsic, "cmp_lt_u64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntULT, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_lt_f32") || StringIsEqualToCString(intrinsic, "cmp_lt_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFCmp(builder->builder, LLVMRealULT, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_le_s8") || StringIsEqualToCString(intrinsic, "cmp_le_s16") ||
        StringIsEqualToCString(intrinsic, "cmp_le_s32") || StringIsEqualToCString(intrinsic, "cmp_le_s64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntSLE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_le_u8") || StringIsEqualToCString(intrinsic, "cmp_le_u16") ||
        StringIsEqualToCString(intrinsic, "cmp_le_u32") || StringIsEqualToCString(intrinsic, "cmp_le_u64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntULE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_le_f32") || StringIsEqualToCString(intrinsic, "cmp_le_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFCmp(builder->builder, LLVMRealULE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_gt_s8") || StringIsEqualToCString(intrinsic, "cmp_gt_s16") ||
        StringIsEqualToCString(intrinsic, "cmp_gt_s32") || StringIsEqualToCString(intrinsic, "cmp_gt_s64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntSGT, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_gt_u8") || StringIsEqualToCString(intrinsic, "cmp_gt_u16") ||
        StringIsEqualToCString(intrinsic, "cmp_gt_u32") || StringIsEqualToCString(intrinsic, "cmp_gt_u64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntUGT, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_gt_f32") || StringIsEqualToCString(intrinsic, "cmp_gt_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFCmp(builder->builder, LLVMRealUGT, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_ge_s8") || StringIsEqualToCString(intrinsic, "cmp_ge_s16") ||
        StringIsEqualToCString(intrinsic, "cmp_ge_s32") || StringIsEqualToCString(intrinsic, "cmp_ge_s64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntSGE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_ge_u8") || StringIsEqualToCString(intrinsic, "cmp_ge_u16") ||
        StringIsEqualToCString(intrinsic, "cmp_ge_u32") || StringIsEqualToCString(intrinsic, "cmp_ge_u64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntUGE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_ge_f32") || StringIsEqualToCString(intrinsic, "cmp_ge_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFCmp(builder->builder, LLVMRealUGE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_eq_i1") || StringIsEqualToCString(intrinsic, "cmp_eq_i8") ||
        StringIsEqualToCString(intrinsic, "cmp_eq_i16") || StringIsEqualToCString(intrinsic, "cmp_eq_i32") ||
        StringIsEqualToCString(intrinsic, "cmp_eq_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntEQ, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_eq_f32") || StringIsEqualToCString(intrinsic, "cmp_eq_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFCmp(builder->builder, LLVMRealUEQ, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_eq_ptr")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        LLVMValueRef lhs = LLVMBuildPtrToInt(builder->builder, arguments[0], LLVMInt64Type(), "");
        LLVMValueRef rhs = LLVMBuildPtrToInt(builder->builder, arguments[1], LLVMInt64Type(), "");
        return LLVMBuildICmp(builder->builder, LLVMIntEQ, lhs, rhs, "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_ne_i8") || StringIsEqualToCString(intrinsic, "cmp_ne_i16") ||
        StringIsEqualToCString(intrinsic, "cmp_ne_i32") || StringIsEqualToCString(intrinsic, "cmp_ne_i64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildICmp(builder->builder, LLVMIntNE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_ne_f32") || StringIsEqualToCString(intrinsic, "cmp_ne_f64")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        return LLVMBuildFCmp(builder->builder, LLVMRealUNE, arguments[0], arguments[1], "");
    }

    if (StringIsEqualToCString(intrinsic, "cmp_ne_ptr")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        LLVMValueRef lhs = LLVMBuildPtrToInt(builder->builder, arguments[0], LLVMInt64Type(), "");
        LLVMValueRef rhs = LLVMBuildPtrToInt(builder->builder, arguments[1], LLVMInt64Type(), "");
        return LLVMBuildICmp(builder->builder, LLVMIntNE, lhs, rhs, "");
    }

    if (StringIsEqualToCString(intrinsic, "ptr_diff")) {
        if (argumentCount != 2) {
            ReportErrorFormat("Intrinsic '%s' expects two arguments", StringGetCharacters(intrinsic));
            return LLVMGetUndef(resultType);
        }

        LLVMValueRef lhs = LLVMBuildPtrToInt(builder->builder, arguments[0], LLVMInt64Type(), "");
        LLVMValueRef rhs = LLVMBuildPtrToInt(builder->builder, arguments[1], LLVMInt64Type(), "");
        return LLVMBuildSub(builder->builder, lhs, rhs, "");
    }

    ReportError("Use of unknown intrinsic");
    return LLVMGetUndef(resultType);
}

LLVMValueRef _IRBuilderBuildLoad(IRBuilderRef Builder, ASTNodeRef Node, const Char* Name) {
    LLVMTypeRef Type = _IRBuilderGetIRType(Builder, Node->type);
    return LLVMBuildLoad2(Builder->builder, Type, Node->irValue, Name);
}

LLVMValueRef _IRBuilderGetConstantSizeOfType(IRBuilderRef builder, ASTTypeRef type) {
    return LLVMSizeOf(_IRBuilderGetIRType(builder, type));
}

LLVMValueRef _IRBuilderImplicitlyConvertValue(IRBuilderRef builder, LLVMValueRef function, LLVMValueRef value,
                                              ASTExpressionRef valueExpression, ASTTypeRef targetType) {
    ASTTypeRef valueType = ASTNodeGetType(valueExpression);
    if (ASTTypeIsEqual(valueType, targetType)) {
        return value;
    }

    Bool isNilExpression = (valueExpression->base.tag == ASTTagConstantExpression &&
                            ((ASTConstantExpressionRef)valueExpression)->kind == ASTConstantKindNil);
    if (isNilExpression && targetType->tag == ASTTagPointerType) {
        return LLVMBuildPointerCast(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
    }

    assert(ASTTypeIsImplicitlyConvertible(valueType, targetType));

    if (valueType->tag == ASTTagPointerType && targetType->tag == ASTTagPointerType) {
        return LLVMBuildPointerCast(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
    }

    if (ASTTypeIsInteger(valueType) && ASTTypeIsInteger(targetType)) {
        Int lhsBitwidth = ASTIntegerTypeGetBitwidth(valueType);
        Bool lhsSigned  = ASTIntegerTypeIsSigned(valueType);
        Int rhsBitwidth = ASTIntegerTypeGetBitwidth(targetType);
        Bool rhsSigned  = ASTIntegerTypeIsSigned(targetType);

        if (!lhsSigned) {
            if (rhsSigned && lhsBitwidth < rhsBitwidth) {
                return LLVMBuildSExtOrBitCast(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
            }

            if (!rhsSigned && lhsBitwidth <= rhsBitwidth) {
                return LLVMBuildZExtOrBitCast(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
            }
        } else if (rhsSigned && lhsBitwidth <= rhsBitwidth) {
            return LLVMBuildSExtOrBitCast(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
        }
    } else if (ASTTypeIsInteger(valueType) && ASTTypeIsFloatingPoint(targetType)) {
        Bool lhsSigned = ASTIntegerTypeIsSigned(valueType);
        if (lhsSigned) {
            return LLVMBuildSIToFP(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
        } else {
            return LLVMBuildUIToFP(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
        }
    } else if (ASTTypeIsFloatingPoint(valueType) && ASTTypeIsFloatingPoint(targetType)) {
        Int lhsBitwidth = ASTFloatingPointTypeGetBitwidth(valueType);
        Int rhsBitwidth = ASTFloatingPointTypeGetBitwidth(targetType);

        if (lhsBitwidth <= rhsBitwidth) {
            return LLVMBuildFPExt(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
        } else {
            return LLVMBuildFPTrunc(builder->builder, value, _IRBuilderGetIRType(builder, targetType), "");
        }
    }

    return value;
}

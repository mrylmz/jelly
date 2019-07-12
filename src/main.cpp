#include <JellyCore/JellyCore.h>

int main(int argc, char **argv) {
    ArrayRef arguments = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(StringRef), argc);
    for (Index index = 0; index < argc; index++) {
        StringRef argument = StringCreate(AllocatorGetSystemDefault(), argv[index]);
        ArrayAppendElement(arguments, &argument);
    }

    ReportInfoFormat("jelly version %s", JELLY_VERSION);
    Int status = CompilerRun(arguments);

    for (Index index = 0; index < ArrayGetElementCount(arguments); index++) {
        StringDestroy(*((StringRef *)ArrayGetElementAtIndex(arguments, index)));
    }

    ArrayDestroy(arguments);
    return status;
}

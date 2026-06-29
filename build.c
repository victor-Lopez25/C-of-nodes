#define VL_BUILD_IMPLEMENTATION
#include "inc/vl_build.h"

/* Resources:
 *  https://github.com/SeaOfNodes
 */

// crazy sea of nodes ? I guess. I don't have a better name for now
#define EXE_NAME "crazy-son"
#define OUTPUT_DIR "bin"

#define EXECUTABLE OUTPUT_DIR "/" EXE_NAME

#if OS_WINDOWS
#define DOT_PROGRAM "D:\\Graphviz\\bin\\dot"
#else
#define DOT_PROGRAM "dot"
#endif

void GeneratePngFilesFromDot(vl_cmd *cmd, const char *directory)
{
  vl_file_paths files = {0};
  if(!VL_ReadEntireDir(directory, &files)) {
    VL_Log(VL_ERROR, "Could not read dot files directory %s", directory);
  }

  VL_Pushd(directory);

  for(size_t i = 0; i < files.count; i++) {
    if(!strcmp(files.items[i], ".") || !strcmp(files.items[i], "..")) continue;

    CmdAppend(cmd, DOT_PROGRAM, "-Tpng", "-O", files.items[i]);
    if(!CmdRun(cmd)) {
      VL_Log(VL_WARNING, "Could not create png from dot file %s", files.items[i]);
    }
  }

  VL_Popd();
}

int main(int argc, char **argv)
{
  VL_GO_REBUILD_URSELF(argc, argv, "inc/vl_build.h");

  bool shouldRun = true;
  for(int i = 1; i < argc; i++) {
    view arg = ViewFromCstr(argv[i]);
    if(ViewEq(arg, VIEW("run"))) {
      shouldRun = true;
    } else if(ViewEq(arg, VIEW("norun"))) {
      shouldRun = false;
    }
  }

  MkdirIfNotExist(OUTPUT_DIR);

  vl_compile_ctx ctx = {
    .debug = true,
    .optimize = Optimize_Nothing,
    .output = EXE_NAME,
    .sourceFiles = VL_GetDaStrSlice("src/main.c"),
    .warnings = true,
    .outputDir = OUTPUT_DIR,
    .includePaths = VL_GetDaStrSlice("inc"),
  };
  vl_cmd cmd = {0};
  if(!VL_CCompile(&cmd, &ctx)) {
    VL_Log(VL_ERROR, "Could not compile %s", EXE_NAME);
    return 1;
  }

  if(shouldRun) {
    CmdAppend(&cmd, EXECUTABLE, "test-programs/first");
    if(!CmdRun(&cmd)) {
      VL_Log(VL_ERROR, "%s exited abnormally", EXECUTABLE);
      return 1;
    }
  }

  // GeneratePngFilesFromDot(&cmd, "bin/steps/TestPeepholeExample");

  return 0;
}

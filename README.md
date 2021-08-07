Documentation to come

# NSTOOLS

NSTOOLS is a collection of tools that are used internally at Agenium Scale. It
consists of

- a C++ library named `ns2` containing
  + a JSON parser with a SAX API
  + a Markdown to HTML converter
  + several utilities functions that are not standard C++ (at least until
    C++17)
  + a fstream-compliant implementation for file reading/writing based on
    `FILE*` and that correctly reports errors to the user
- a configure system named `nsconfig` that is able to generate
  + POSIX Makefiles
  + GNU Makefiles
  + Microsoft Makefiles
  + ninja files (default)
- a small C utility to spawn resilient processes through SSH.
- a small Dolibarr module that generates workloads reports.
- a set of installation script for Linux systems.
- a program to block slowloris attacks.

# The `ns2` C++ library

It is located in the `ns2` directory and can be built for both Linux and
Windows using `nsconfig`. As no documentation is available yet here are
commented pieces of the listing of [`md2html.cpp`](https://github.com/agenium-scale/nsimd/blob/master/doc/md2html.cpp).

```c++
[...]

#include <ns2.hpp> // That's all you need for the entire library.

#include <stdexcept>

[...]

int main(int argc, char **argv) {
  [...]

  std::string const input_filename = argv[1];
  std::string const output_filename = argv[2];

  // Open a file for reading, there is no mode, to other arguments
  // for the ctors of ns2::ifile_t. It simply open the files
  // in read and binary mode using fopen(3).
  ns2::ifile_t input_file(input_filename);
  
  // Same here for writing into a file.
  ns2::ofile_t output_file(output_filename);

  // Accessing the underlying FILE* is done via the c_file() member
  // function. Other helper functions are provided. For example the
  // filename() member function gives the filename of the underlying
  // file. Note also that when an error occurs an exception is thrown.
  // The exception will be of type ns2::file_error_t and inherits from
  // std::exception. The what() of the exception contains useful
  // informations to help the user and the developper: the filename
  // followed by the strerror(errno) string.

  [...]
  
  // Invoking the Markdown convertor is simple. One first needs
  // to fill a markdown_infos_t struct to inform the output (even
  // if for now only HTML is supported) and callbacks when the
  // parser will encounter links and macros. For more details
  // please read the complete source code of md2html.cpp which is
  // not very long.
  ns2::markdown_infos_t markdown_infos(ns2::HTML, callback_macro,
                                       callback_link, true);

  // Finally the compile_markdown function does the translation.
  // It reads from a std::istream object and writes into a
  // std::ostream object.
  ns2::compile_markdown(&input_file, &output_file, markdown_infos);

  return 0;
}
```

`Ns2` contains many more functions. Looking at the
[`tests`](https://github.com/agenium-scale/nstools/tree/master/ns2/tests)
directory will show you how to use the functions. The prototypes of functions
[here](https://github.com/agenium-scale/nstools/tree/master/ns2/include/ns2)
will help you to use them.

# Nsconfig

```
usage: nsconfig [OPTIONS]... DIRECTORY
Configure project for compilation.

  -v              verbose mode, useful for debugging
  -nodev          Build system will never call nsconfig
  -DVAR=VALUE     Set value of variable VAR to VALUE
  -list-vars      List project specific variable
  -GBUILD_SYSTEM  Produce files for build system BUILD_SYSTEM
                  Supported BUILD_SYSTEM:
                    make       POSIX Makefile
                    gnumake    GNU Makefile
                    nmake      Microsot Visual Studio NMake Makefile
                    ninja      Ninja build file (this is the default)
                    list-vars  List project specific variables
  -oOUTPUT        Output to OUTPUT instead of default
  -suite=SUITE    Use compilers from SUITE as default ones
                  Supported SUITE:
                    gcc       The GNU compiler collection
                    msvc      Microsoft C and C++ compiler
                    llvm      The LLVM compiler infrastructure
                    armclang  Arm suite of compilers based on LLVM
                    xlc       IBM suite of compilers
                    fcc_trad_mode
                              Fujitsu compiler in traditional mode
                    fcc_clang_mode
                              Fujitsu compiler in clang mode
                    emscripten
                              Emscripten suite for compiling into JS
                    icc       Intel C amd C++ compiler
                    rocm      Radeon Open Compute compilers
                    oneapi    Intel oneAPI compilers
                    cuda, cuda+gcc, cuda+clang, cuda+msvc
                              Nvidia CUDA C++ compiler
  -comp=COMMAND,COMPILER[,PATH[,VERSION[,ARCHI]]]
                  Use COMPILER when COMMAND is invoked for compilation
                  If VERSION and/or ARCHI are not given, nsconfig will
                  try to determine those. This is useful for cross
                  compiling and/or setting the CUDA host compiler.
                  COMMAND must be in { cc, c++, gcc, g++, cl, icc, nvcc,
                  hipcc, hcc, clang, clang++, armclang, armclang++,
                  cuda-host-c++, emcc, em++ } ;
                  VERSION is compiler dependant. Note that VERSION
                  can be set to only major number(s) in which case
                  nsconfig fill missing numbers with zeros.
                  Supported ARCHI:
                    x86      Intel 32-bits ISA
                    x86_64   Intel/AMD 64-bits ISA
                    armel    ARMv5 and ARMv6 32-bits ISA
                    armhf    ARMv7 32-bits ISA
                    aarch64  ARM 64-bits ISA
                    ppc64el  PowerPC 64-bits little entian
                    wasm32   WebAssembly with 32-bits memory indexing
                    wasm64   WebAssembly with 64-bits memory indexing
                  Supported COMPILER:
                    gcc, g++              GNU Compiler Collection
                    clang, clang++        LLVM Compiler Infrastructure
                    emcc, em++            Emscripten compilers
                    msvc, cl              Microsoft Visual C++
                    armclang, armclang++  ARM Compiler
                    xlc, xlc++            IBM Compiler
                    icc                   Intel C/C++ Compiler
                    dpcpp                 Intel DPC++ Compiler
                    nvcc                  Nvidia CUDA compiler
                    hipcc                 ROCm HIP compiler
                    fcc_trad_mode, FCC_trad_mode
                                          Fujitsu C and C++ traditionnal
                                          compiler
                    fcc_clang_mode, FCC_clang_mode
                                          Fujitsu C and C++ traditionnal
                                          compiler
  -prefix=PREFIX  Set path for installation to PREFIX
  -h, --help      Print the current help

NOTE: Nvidia CUDA compiler (nvcc) needs a host compiler. Usually on
      Linux systems it is GCC while on Windows systems it is MSVC.
      If nvcc is chosen as the default C++ compiler via the -suite
      switch, then its host compiler can be invoked in compilation
      commands with 'cuda-host-c++'. The latter defaults to GCC on Linux
      systems and MSVC on Windows systems. The user can of course choose
      a specific version and path of this host compiler via the
      '-comp=cuda-host-c++,... parameters. If nvcc is not chosen as the
      default C++ compiler but is used for compilation then its default
      C++ host compiler is 'c++'. The latter can also be customized via
      the '-comp=c++,...' command line switch.
```

## build.nsconfig file reference

Each line can be prefixed by `[xxx]` where `xxx` can indicates the system on
which the line will be parsed or indicates what kind of shell command
translation will be done. More precisely if `xxx` is

- `W` then the line is only available on Windows systems.
- `L` then the line is only available on Linux systems.
- `T` then translate the shell commands and issue an error when cannot.
- `P` then translate the shell commands when possible otherwise, copy as-is.
- `R` then copy as-is shell commands.

Note that `xxx` can contain both `W` or `L` with `T`, `P` or `R`. One can
put a variable and if it expanded as non empty string then the rest of the
line will be translated, if it is expanded as the empty string then the line
won't be translated.

### `disable_all`

Disable automatic generation of the `all` rule.

### `disable_clean`

Disable automatic generation of the `clean` rule.

### `disable_update`

Disable automatic generation of the `update` rule.

### `disable_package`

Disable automatic generation of the `package` rule.

### `disable_install`

Disable automatic generation of the `install` rule.

### `(set | lambda) var = value`

Set value of variable `var` to `value`. `Value` can be one of the following.

* `@source_dir`: resolve to the absolute directory where the `build.nsconfig`
                 file is.
* `@build_dir`: resolve to the absolute directory where the project is being
                built.
* `@obj_ext`: object file extension, usually ".o" on Linux and ".obj" on
              Windows.
* `@asm_ext`: assembly file extension, usually ".s" on Linux and ".asm" on
              Windows.
* `@static_lib_ext`: static library extension, usually ".a" on Linux and ".lib"
                     on Windows.
* `@shared_lib_ext`: shared library extension, usually ".so" on Linux and
                     ".dll" on Windows.
* `@shared_link_ext`: extension of the file to pass the compiler in order to
                      link against a library, usually ".so" on Linux and ".lib"
                      on Windows.
* `@exe_ext`: extension of an executable, usually "" on Linux and ".exe" on
              Windows.
* `@prefix`: Installation prefix as given by the `-prefix` command-line switch.

When a lambda is set the '*' contained in `value` will be replaced by anything
non-empty when trying to find the variable name. Here is an example:

```
ifnot_set "Parameter (can be 0, 1, ... 9)" param = 0

lambda cmd_for_0 = foo
lambda cmd_for_5 = bar
lambda_cmd_for_* = default_value

set cmd = ${lambda_for_$param$}
```

The `cmd` variables will have value `foo` when `param` is `0`, value `bar` when
`param` is `5` and finally value `default_value` for any non-empty value of
param.

### `package_name`

Set package name for the project. On Linux the automatic generation of the
`package` target will produce a `.tar.bz2` file on Linux while on Windows
a `.zip` file will be produced. The content of the archive is determined by
the `install` commands.

### `install_file file1 ... fileN relative_path`

Tell nsconfig to add `file1`, ..., `fileN` for package creation and project
installation. For installation, nsconfig will install the given files into
`prefix/relative_path` where `prefix` is the installation prefix as given by
the command-line switch `-prefix`. For package creation, nsconfig will copy the
given files into the archive at `package_name/relative_path` where
`package_name` is the package name given by the `package_name` command.

### `install_dir dir1 ... dirN relative_path`

Same as `install_file` except that `dir1`, ..., `dirN` must be directories.

### `ifnot_set description var = value`

Same as `set` above except that `var` is set only if has not been set yet and
map the given `description` to  `var`. The idea is that `ifnot_set` is mainly
used for setting variables that have not been defined on command line by the
end-user with a default value. Therefore we force the programmer to give
a one-liner help that is obtained by `nsconfig -list-vars`.

### `glob var = glob1 glob2 ... globN`

Set value of variable `var` to the list of files computed from the globbing
`glob1`, `glob2`, ..., `globN`.

### `getenv var = env`

Set value of variable `var` to the value of the environment variable `env`.

### `build_file output (deps | autodeps) dep1 dep2 ... depN`

Generate a target with its dependencies. The target will be named `output` and
its dependencies `dep1`, `dep2`, ..., `depN`. When `autodeps` is used, nsconfig
will generate extra code for handling automatic C/C++ header dependencies. This
feature is only available with Ninja combined with GCC/Clang or MSVC.

### `phony output (deps | autodeps) dep1 dep2 ... depN`

Generate a phony target with its dependencies. The target will be named
`output` and its dependencies `dep1`, `dep2`, ..., `depN`. When `autodeps` is
used, nsconfig will generate extra code for handling automatic C/C++ header
dependencies. This feature is only available with Ninja combined with GCC/Clang
or MSVC.

### `build_files [optional] var foreach globs as format [(deps | autodeps) [dep1 dep2 ... depN]]`

`Globs` can be one or more space-separated filenames, globbing expressions or
command line calls. Globing expression must be prefixed by `glob:`. Command
line calls must be prefixed by `popen:`. For exemple if you want to glob all
C++ source files and then get symbols in each of them:

    build_files objects foreach glob:*.cpp as %b.o deps @item
      c++ @item -c -o @out

    build_files symbols foreach %objects.files as %b.o deps @item
      nm @item > @out

For each file given in `globs` generate a target whose name follows `format`
whose details are given below and dependencies are the file specified by
`@item`, `dep1`, `dep2`, ..., `depN`. If you want to get the list of inputs
from a command.

    build_files objects foreach popen:"python2 script.py --get-list" \
                as %b.txt deps @item
      c++ @item -c -o @out

    build_files symbols foreach $objects.files as %b.txt deps @item
      nm $item > $out

To get the list of files from the given command, nsconfig treats stdout and
stderr only when the command return code is 0, in the following way:

- At most one file must be specified by line.
- Empty lines are ignored.
- Lines beginning with a '#' are ignored.
- Filenames can be split in half by a ';' say `/dir1/dir2;dir3/file` in which
  case input will be `/dir1/dir2/dir3/file` and output will be
  `[prefix]dir3.file[suffix]`.
- If file is not split in half say `/dir1/dir2/dir3/file` in which case input
  will be `/dir1/dir2/dir3/file` and output will be `[prefix]file[suffix]`.

If no file is found from the globbings then no error is reported if `optional`
is given.

After havig parsing this rule the following variables will be available in
the nsconfig file:

- `[var].files`: the list of all output files (or targets).

When `autodeps` is used, nsconfig will generate extra code for handling
automatic C/C++ header dependencies. This feature is only available with Ninja
combined with GCC/Clang or MSVC.

The `format` follows a printf like syntax. The output name of each generated
file will be format except that:
- `%b` will be replaced by the basename of the input (with the extension),
- `%r` will be replaced by the root filename of the input and
- `%e` will be replaced by the extension of the input.

### `find_exe [optional] var = exe path1 path2 ... pathN`

Find an executable named `exe` (no need for the ".exe" extension on Windows)
in `path1`, `path2`, ..., `pathN` first then in the directories from the `PATH`
environment variable.

After havig parsing this rule the following variables will be available in
the nsconfig file:

- `[var]`: the full path for the executable or empty if it was not found.
- `[var].dir`: the directory where the executable was found or empty if it
  was not found.

### `find_header [optional] var = header path1 path2 ... pathN`

Find a header file named `header` (no need for the extension) in `path1`,
`path2`, ..., `pathN`.

After havig parsing this rule the following variables will be available in
the nsconfig file:

- `[var].flags`: the flags to pass to a compiler in order to compile with
  this header, usually contains "-DHAS_HEADER -Iheader_dir"; or empty if it
  was not found.
- `[var].cflags`: same as `[var]_flags`.
- `[var].dir`: the directory where the header was found or empty if it
  was not found.

### `find_lib [optional] [dynamic | static] var = lib.h name path1 path2 ... pathN`

Find a library named `name` (no need for the "lib" prefix or the extension)
in paths `path1`, `path2`, ..., `pathN` whose corresponding header file is
named `lib.h`. Note that the binary will be search in `pathX/lib` and
`pathX/lib64` while the header will be searched in `pathX/include`.

After havig parsing this rule the following variables will be available in
the nsconfig file:

- `[var].header_dir`: the directory where the header was found or empty if it
  was not.
- `[var].lib_dir`: the directory where the binary was found or empty if it was
  not.
- `[var].cflags`: the compilation flags to give a compiler in order to compile
  against the library, usually contains "-DHAS_NAME -Iinclude/dir" or empty if
  it was not found.
- `[var].ldflags`: the linker flags to give a linker in order to compile
  against the library, usually contains "-Llib/dir -lname" or empty if it was
  not found.
- `[var].flags`: the concatenation of `[var].cflags` and `[var].ldflags`.

### `echo some test`

Print to stdout `text`, useful for debugging.

### `include file1 file2 ... fileN`

Includes "à la C" the content of the files `file1`, `file2`, ..., `fileN` into
the calling nsconfig file.

## Nsconfig shell commands reference

The following shell commands are recognized by nsconfig and will translated
into the OS native shell (sh for Linux and cmd.exe for Windows). Commands
can be chained with `&&`, `||` and `;` which have their usual meanings:

- `cmd1 && cmd2`: execute `cmd1` and then execute `cmd2` only if `cmd1` has
  returned a zero code.
- `cmd1 || cmd2`: execute `cmd1` and then execute `cmd2` only if `cmd1` has
  returned a nonzero code.
- `cmd1 ; cmd2`: execute `cmd1` and then execute `cmd2` whatever is the return
  code of `cmd1`.

You can also use redirections `>`, `>>` and `|` with their usual meanings:

- `cmd > file`: redirect output of `cmd` to `file` by replacing its original
  content.
- `cmd >> file`: redirect output of `cmd` to `file` by appending to its
  original content.
- `cmd1 | cmd2`: redirect output of `cmd1` to input of `cmd2`.

Any restrictions in the following commands come from the OS native shells. For
example cmd.exe cannot touch several files in one command so only one file
is supported fir this command by nsconfig.

By default commands will be translated and an error will be thrown if nsconfig
cannot translate one. If the use of other commands is needed then use the
line prefixes `[*:P]` or `[*:R]`.

### `cd dir`

Change current directory to `dir`.

### `touch file`

Touch `file`.

### `rm [-r] path1 path2 ... pathN`

Force remove (recursively if `-r` is present) `path1`, `path2`, ..., `pathN`.

### `cp [-r] src dest`

Force copy (recursively if `-r` is present) `src`, to `dest`.

### `mkdir dir`

Force create the directory `dir`. Note that any intermediate directory will
be created.

### `cat file1 file2 ... fileN`

Output the content of `file1`, `file2`, ..., `fileN` to stdout.

### `echo text`

Output `text` to stdout

### `mv path dest`

Move `path` to `dest`.

### `if str1 op str2 ( cmd1 ) [ else ( cmd2 ) ]`

`Op` can be either `==` or `!=`. Execute `cmd1` if the condition `str1 op str2`
is true. If the condition is false and an `else` is present then execute
`cmd2`.

### `ar rcs out file1 file2 ...`

Create the static library `out` with input files `file1`, `file2`, ...

### `compiler options file1 file2 ... fileN`

`Compiler` can be one of the following:

- `cc`
- `c++`
- `gcc`
- `g++`
- `clang`
- `clang++`
- `msvc`
- `icc`

If the compiler is `cc` or `c++` nsconfig will try to find the native compiler
of the system it is running on. On Linux, Clang will be the first choice then
GCC. On Windows MSVC will be the first choice, then Clang and then GCC.

Compile `file1`, `file2`, ..., `fileN`. Several options can be given to the
compiler:

- `-std=c89`: compile using the C89 standard.
- `-std=c99`: compile using the C99 standard.
- `-std=c++98`: compile using the C++98 standard.
- `-std=c++03`: compile using the C++03 standard.
- `-std=c++11`: compile using the C++11 standard.
- `-std=c++14`: compile using the C++14 standard.
- `-std=c++17`: compile using the C++17 standard.
- `-std=c++20`: compile using the C++20 standard.
- `-O1`: compile using level 1 optimizations.
- `-O2`: compile using level 2 optimizations.
- `-O3`: compile using level 3 optimizations.
- `-ffast-math`: compile using non C/C++/IEEE754 conforming optimizations
- `-g`: compile with debug informations.
- `-c`: compile only, do not link.
- `-S`: generate assembly code.
- `-o file`: output to `file`.
- `-Wall`: compile with all warnings.
- `-fPIC`: emit position independant code.
- `-msse`: enable SSE extension.
- `-msse2`: enable SSE2 extension.
- `-msse3`: enable SSE3 extension.
- `-mssse3`: enable SSSE3 extension.
- `-msse41`: enable SSE41 extension.
- `-msse42`: enable SSE42 extension.
- `-mavx`: enable AVX extension.
- `-mavx2`: enable AVX2 extension.
- `-mavx512_knl`: enable AVX512 extensions as found on KNL chips.
- `-mavx512_skylake`: enable AVX512 extensions as found on Skylake chips.
- `-mneon128`: enable ARMv7 NEON 128 bits extensions.
- `-maarch64`: enable Advanced SIMD extensions.
- `-mvmx`: enable PowerPC VMX extensions.
- `-mvsx`: enable PowerPC VSX extensions.
- `-msve`: enable SVE extensions.
- `-msve128`: enable SVE extensions for fixed sized vectors of 128 bits.
- `-msve256`: enable SVE extensions for fixed sized vectors of 256 bits.
- `-msve512`: enable SVE extensions for fixed sized vectors of 512 bits.
- `-msve1024`: enable SVE extensions for fixed sized vectors of 1024 bits.
- `-msve2048`: enable SVE extensions for fixed sized vectors of 2048 bits.
- `-msm_35`: compile kernels for CUDA with device capability 3.5
- `-msm_50`: compile kernels for CUDA with device capability 5.0
- `-msm_53`: compile kernels for CUDA with device capability 5.3
- `-msm_60`: compile kernels for CUDA with device capability 6.0
- `-msm_61`: compile kernels for CUDA with device capability 6.1
- `-msm_62`: compile kernels for CUDA with device capability 6.2
- `-msm_70`: compile kernels for CUDA with device capability 7.0
- `-msm_72`: compile kernels for CUDA with device capability 7.2
- `-msm_75`: compile kernels for CUDA with device capability 7.5
- `-mfma`: enable FMAs.
- `-mfp16`: enable native FP16 support.
- `-fopenmp`: enable handling of OpenMP directives.
- `-shared`: creates a dynamic linked library.
- `-lpthread`: adds support for multithreading with the pthreads library.
- `-lm`: links against the Math library.
- `-Idir`: add the `dir` to the list of directories to be searched for headers.
- `-lfoo`: links against the foo library (libfoo.so on Linux, libfoo.dll on
  Windows).
- `-l:foo.ext`: links against the `foo.ext` file.
- `-Ldir`: add `dir` to the list of directories to be searched for `-l`.
- `-L$ORIGIN`: tell the linker to link against libraries in the same folder
  as the binary.
- `-fdiagnostics-color=always`: Use color in diagnostics.
- `--version`: display compiler version and ignore all other flags.
- `-fno-omit-frame-pointer`: Keep the frame pointer in a register for
                             functions.
- `-vec-report`: Print to stdout full report on autovectorization of loops.

# Sshjob

This tiny helper program has for sole purpose to spawn resilient processes
through SSH that can be killed easily.

For Linuxes there is not really any need for that since many solutions already
exist: nohup, tmux, screen, good use of the shell... For more details see:
<https://stackoverflow.com/questions/29142/getting-ssh-to-execute-a-command-in-the-background-on-target-machine>
Howver by default on Linux processes spawned through SSH are not part of a
process group or session and therefore cannot be killed that easily. That's
why on Linux sshjob create a process session and group before spawning any
process.

The plus of this C program is the support of the same functionnality for
Microsoft Windows systems using Microsoft OpenSSH port (available at github:
<https://github.com/PowerShell/openssh-portable>).

## How it works?

On Linux, the job is created using setsid(2) and the usual fork(2) + execv(2).
Moreover file descriptors 0, 1 and 2 are closed for the job so that is
detached from SSH. On Windows the situation is a little more complex. We use
a 2-stage process as we do not have found a simplier way:

- stage1: We create a process that does not inherit any handle to be detached
  from SSH with the `CREATE_BREAKAWAY_FROM_JOB` flag so that the new process
  is not within the SSH job object and hence is not killed by SSH.

- stage2: We create a new job object and assign the process from stage1 to
  the newly created job object. The stage1 process then creates a new job
  that finally executes what the user wants. The resulting process is created
  with two particularities:

  1. The created job object must have a handle that can be inherited. Indeed
     if it is not the case then the job object can never be referenced by
     any other process and therefore cannot be killed. A handle to the job
     must be alive.

  2. The final process is created and inherits all the handles of the stage1
     process including the handle to the job object.

We do not provide any way of compiling it as the main use of this C program
is to be copied through network to computers part of the CI and compiled there
quickly. This program has no dependencies and is as portable as possible.

## Compilation on Linux and Windows

Windows: `cl /Ox /W3 /D_CRT_SECURE_NO_WARNINGS sshjob.c`

Unix: `cc -O2 -Wall sshjob.c -o sshjob`

# Timetable

Timetable is a small [Dolibarr](https://www.dolibarr.org) module that generates
CSVs containing the team workload summary. One chooses the period that the
report has to cover and click "Generate". The CSV will contain a 2D table
with dates in increasing order and team members sorted by their lastnames.
Then each cell will contain the name of the project they declared time spent
on. If on the same day they workd on several projects, then the corresponding
cell will contain "PROJECT1 + PROJECT2".

When a project title contains the substring "meta" then timetable will take
the label of the task the team member declared time spent on instead of the
corresponding project title.

# Installation scripts

The `installation-scripts` contains bash scripts for the installation of
several software. They are not heavely tested for other shells and may contains
bugs. Each installation script produces a `modulefile` for the `module`
command. The `modulefile` will be installed into the directories provided by
the `module` command found in the `$PATH` when the install script is launched.

## Installing GCC

Installing GCC is done by the following.

```sh
bash install-gcc.sh (trunk | VERSION)
```

When passing `trunk`, it will download and install the last SVN revision of
GCC. When passing the version number, the script will download the specified
version of GCC to install it as in the example below for GCC 9.2.

```sh
bash install-gcc.sh 9.2.0
```

## Installing Clang

Installing Clang is done by the following.

```sh
bash install-clang.sh VERSION
```

Passing the version number, the script will download the specified
version of Clang/LLVM to install it as in the example below for Clang 9.0.0.

```sh
bash install-clang.sh 9.0.0
```

## Installing CMake

Installing CMake is done by the following.

```sh
bash install-cmake.sh
```

It will download and install CMake version 3.10.0. The version is hard-coded
inside the script.

## Installing Python 2

Installing Python 2 is done by the following.

```sh
bash install-python27.sh
```

It will download and install Python version 2.7.14. The version is hard-coded
inside the script.

## Installing HTOP

Installing HTOP is done by the following.

```sh
bash install-htop.sh
```

It will download and install HTOP version 2.0.2. The version is hard-coded
inside the script.

## Installing patchelf

Installing patchelf is done by the following.

```sh
bash install-patchelf.sh
```

It will download and install patchelf version 0.9. The version is hard-coded
inside the script.

# Http2s

Http2s is a program that acts as a small web server and blocks slowloris
attacks. It does not support `https`. It listen to `http` requests and
judge whether they are attacks or not. If yes they are simply closed otherwise
a redirect is sent to the client to the proper website. It only work on Linux
for now and may contain bugs. There are no proper testing but it has worked
in production for several years without being interrupted.

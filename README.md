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

Nsconfig is the replacement of CMake at Agenium Scale. We used to use CMake but
it did fit our needs. After having considered other build systems our choice
was made to write yet another one that suits our needs. This build system may
or may not be the right one for you. It was not written to replace CMake or the
autotools worldwide. It was written for our needs only. As no dedicated pages
for the documentation is ready for nstools and for nsconfig, what follows is the
documentation of nsconfig.

## Overview

Nsconfig is a replacement for CMake and other so called meta build systems.
You have to think of nsconfig as a Makefile translator rather than a Makefile
generator.

For now it supports output to POSIX Makefile, MSVC NMake Makefiles,
GNU Makefiles and Ninja build file. You write a "build.nsconfig" file and then
nsconfig will parse it and generate the requested make/nmake/ninja file.

nsconfig is designed to be portable accross Linux and Windows and to support
several compilers: GCC/Clang, MSVC and ICC.

    set exe    = @exe_ext
    set o      = @obj_ext
    set root   = @source_dir
    set cflags = -Wall -O2

    # Compile each .cpp file into an object file
    build_files foo foreach glob:%root%/*.cpp as %b$o
            c++ $cflags @item -c -o @out
 
    # Link all object files together
    build_file foo$exe deps $foo.files
            c++ $foo_files -o @out

Once your build.nsconfig is written simply call nsconfig as you would CMake.

    $ mkdir build
    $ cd build
    $ nsconfig ..
    $ ninja

By default three rules will be generated:

- `all`: will by default generates all non-phony targets.
- `clean`: will clean all built files by non-phony targets.
- `update`: will re-execute nsconfig with the arguments that were used to
  generate the Makefile/Ninja build file.

## Nsconfig command line switches

- `-Ggenerator`: Choose which generator to use. Supported generators:
  + `make`: POSIX Makefile
  + `gnumake`: GNU Makefile
  + `nmake`: Microsot Visual Stusdio NMake Makefile
  + `ninja`: Ninja build file (this is the default)
  + `list-vars`: List project specific variables

- `-Dvar=value`: Affect value `value` to variable named `var` for the
  build.nsconfig files. It can be accessed with `%var%`.

- `-list-vars`: List variables specific to a project. The list of variables
  simply consists of those that are in ifnot_set.

- `-ooutput`: Instead of writing its output to the default file (`build.ninja`
  for Ninja, Makefile for the other generators) write the output to the file
  named `output`.

- `-comp=type`: Tell nsconfig that the default C and C++ compilers are of type
  `type`. This is a shortcut for `-ccomp=` and `cppcomp=`. The type must be one
  of the following:
  + gcc: GNU Compiler Collection
  + clang: LLVM Compiler Infrastructure
  + msvc: Microsoft Visual C++
  + armclang: ARM Compiler
  + icc: Intel C/C++ Compiler

- `-ccomp=type,path`: Tell nsconfig that the default C compiler is of type
  `type` can be found at `path` (which can be a relative or an absolute path).
  All invocations of `cc` in any command that has to be translated will refer
  to the specified compiler. The type must be one of the following:
  + gcc: GNU Compiler Collection
  + clang: LLVM Compiler Infrastructure
  + msvc: Microsoft Visual C++
  + armclang: ARM Compiler
  + icc: Intel C/C++ Compiler

- `-cppcomp=type,path`: Tell nsconfig that the default C++ compiler is of type
  `type` can be found at `path` (which can be a relative or an absolute path).
  All invocations of `c++` in any command that has to be translated will refer
  to the specified compiler. The type must be one of the following:
  + gcc: GNU Compiler Collection
  + clang: LLVM Compiler Infrastructure
  + msvc: Microsoft Visual C++
  + armclang: ARM Compiler
  + icc: Intel C/C++ Compiler

- `-nodev`: Deactivate the fact that all rules depends on the Makefile or
  build.ninja file itself to allow automatic regeneration. This is useful if
  you intend to provide the Makefile or build.ninja to a third party.

- `-prefix=prefix`: Tell nsconfig that prefix for installation of the project
  is `prefix`. If none is given default is `/opt/local` on Linux and
  `C:\Program Files` on Windows.

- `--help`: Print a quick help on stdout.

## build.nsconfig file reference

Each line can be prefixed by `[S:TR]` where `S` indicates the system on
which the line will be parsed and `TR` indicates what kind of sheel command
translation is requested.

`S` can have the following values:

- `W`: line only available on Windows.
- `L`: line only available on Linux
- `*`: line available on all systems.

`TR` can have the following values:

`T`: translate shell commands and stop when cannot.
`P`: translate shell commands when possible otherwise, copy as-is.
`R`: copy as-is shell commands.

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

### `set var = value`

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
- `-msve`: enable SVE extensions.
- `-msve128`: enable SVE extensions for fixed sized vectors of 128 bits.
- `-msve256`: enable SVE extensions for fixed sized vectors of 256 bits.
- `-msve512`: enable SVE extensions for fixed sized vectors of 512 bits.
- `-msve1024`: enable SVE extensions for fixed sized vectors of 1024 bits.
- `-msve2048`: enable SVE extensions for fixed sized vectors of 2048 bits.
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

# Sshdetach

This tiny helper program has for sole purpose to spawn resilient processes
through SSH.

For Linuxes there is not really any need for that since many solutions already
exist: nohup, tmux, screen, good use of the shell... For more details see:
<https://stackoverflow.com/questions/29142/getting-ssh-to-execute-a-command-in-the-background-on-target-machine>

The plus of this C program is the support of the same functionnality for
Microsoft Windows systems using Microsoft OpenSSH port (available at github:
<https://github.com/PowerShell/openssh-portable>).

## How it works?

On Linux, nothing new. Double fork + close all `std*` and done.

On Windows it seems that sshd does not do a simple launch via `CreateProcess`
but creates a `JobObject`
(<https://docs.microsoft.com/en-us/windows/desktop/procthread/job-objects>).
As a consequence any spawned process via CreateProcess belongs to this
`JobObject`. Then as soon as the command executed by sshd finishes, all its
child processes are killed because they all belong to the same `JobObject`. The
trick is to create a child process that get out of the `JobObject`. This is the
job of this C program. It is done with the `CREATE_BREAKAWAY_FROM_JOB` flag
passed to `CreateProcess`. This seems to work and will do as long as sshd does
create a `JobObject` with the `JOB_OBJECT_LIMIT_BREAKAWAY_OK` flag.

## Compilation on Linux

Make sure that build essential tools are available.

```sh
make -f Makefile.nix
```

## Compilation on Windows

Make sure that a Visual Studio Prompt is available.

```sh
nmake /F Makefile.win
```

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

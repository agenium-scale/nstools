" MIT License
"
" Copyright (c) 2019 Agenium Scale
"
" Permission is hereby granted, free of charge, to any person obtaining a copy
" of this software and associated documentation files (the "Software"), to deal
" in the Software without restriction, including without limitation the rights
" to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
" copies of the Software, and to permit persons to whom the Software is
" furnished to do so, subject to the following conditions:
"
" The above copyright notice and this permission notice shall be included in all
" copies or substantial portions of the Software.
"
" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
" IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
" FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
" AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
" LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
" OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
" SOFTWARE.

" Syntax highlight for nsconfig files

if exists("b:current_syntax")
  finish
endif
let b:current_syntax = "nsconfig"

" Keywords

sy iskeyword 48-57,192-255,@,@-@,_

sy keyword nsconfig_statements
  \ disable_all
  \ disable_update
  \ disable_clean
  \ disable_install
  \ disable_package
  \ set
  \ ifnot_set
  \ lambda
  \ glob
  \ popen
  \ ifnot_glob
  \ getenv
  \ build_file
  \ phony
  \ build_files
  \ find_exe
  \ find_lib
  \ find_header
  \ echo
  \ include
  \ install_dir
  \ install_file
  \ package_name
  \ begin_translate_if
  \ end_translate

sy keyword nsconfig_types
  \ optional
  \ deps
  \ as
  \ autodeps
  \ foreach
  \ dynamic
  \ static
  \ import

sy keyword nsconfig_constants
  \ @source_dir
  \ @build_dir
  \ @obj_ext
  \ @asm_ext
  \ @static_lib_ext
  \ @shared_lib_ext
  \ @shared_link_ext
  \ @exe_ext
  \ @in
  \ @item
  \ @out
  \ @make_command
  \ @prefix
  \ @ccomp_name
  \ @ccomp_path
  \ @cppcomp_name
  \ @cppcomp_path

" Paragraphs

sy region nsconfig_comment
  \ start="#"
  \ end="\n"
  \ contains=@Spell

sy region nsconfig_string
  \ start="\""
  \ end="\""
  \ contains=@Spell

sy region nsconfig_string
  \ start="\\\""
  \ end="\\\""
  \ contains=@Spell

sy region nsconfig_expand_vars
  \ start="\$"
  \ end="[ \n\t\r\$]"

sy region nsconfig_expand_vars
  \ start="${"
  \ end="}"

" Colors

hi def link nsconfig_comment Comment
hi def link nsconfig_statements Type
hi def link nsconfig_types Statement
hi def link nsconfig_constants Constant
hi def link nsconfig_string Constant
hi def link nsconfig_expand_vars Macro

setlocal noexpandtab

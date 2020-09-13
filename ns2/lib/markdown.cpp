// MIT License
//
// Copyright (c) 2020 Agenium Scale
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// This is a very quick and very dirty implementation of a subset of markdown
// converter so why does it exists? C++ implementations are old and not
// maintained anymore:
//
// Last commit:
// - Feb 2016 <https://github.com/ali-rantakari/peg-markdown-highlight>
// - Apr 2014 <https://sourceforge.net/projects/cpp-markdown/>
// - May 2012 <https://github.com/siuying/libupskirt> (archived)
//
// Those that are maintained are badly written using regex, no error reporting
// and require at least C++14:
//
// Last commit:
// - Feb 2019 <https://github.com/progsource/maddy>
//
// There are C implementation out there that seem to be maintained but mixing
// C and C++ requires a wrapper class for proper resources management. Plus
// those C libraries do not necessarily suit our needs:
//
// Last release:
// - Aug 2018 <http://www.pell.portland.or.us/~orc/Code/discount/>
//
// In addition, we need to parse specific stuff.
//
// Anyway this implementation tries to abstract the output format even if for
// now it supports only HTML output. It does support only a subset of Markdown
// and further efforts to improve this should be carefully considered. Indeed
// it can worth the effort to write an external Markdown library. It depends
// on the time needed to do the wanted modification(s).

#include <ns2.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef _MSC_VER
#include <unistd.h>
#endif

namespace ns2 {

// ----------------------------------------------------------------------------

// clang-format off

enum markup_t
{ P, UL, OL, LI, TABLE, INDENTED_PRE, PRE, H1, H2, H3, H4, H5, H6, B, EM, CODE, MATHJAX };

static std::array<int, 19> markup_indents = {
{ 0,  2,  3,  0,     0,            4,   0,  0,  0,  0,  0,  0,  0, 0,  0,    0,       0 }};

// clang-format on

static std::array<markup_t, 3> indented_markups = {{UL, OL, INDENTED_PRE}};

static bool is_indented_markup(markup_t const &markup) {
  return std::find(indented_markups.begin(), indented_markups.end(), markup) !=
         indented_markups.end();
}

// ----------------------------------------------------------------------------
// Translations of begins and ends of blocks

static std::array<std::array<const char *, 17>, 1> t_begin = {
    {{"<p>", "<ul>\n", "<ol>\n", "<li>", "<table>\n", "<pre><code>",
      "<pre><code>", "<h1>", "<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<b>",
      "<em>", "<code>", "\\("}}};

static std::array<std::array<const char *, 17>, 1> t_end = {
    {{"</p>", "</ul>\n", "</ol>\n", "</li>\n", "</tbody>\n</table>\n",
      "</code></pre>", "</code></pre>", "</h1>\n", "</h2>\n", "</h3>\n",
      "</h4>\n", "</h5>\n", "</h6>\n", "</b>", "</em>", "</code>", "\\)"}}};

// ----------------------------------------------------------------------------

const char *endl = "\n";

// ----------------------------------------------------------------------------
// for debugging only

/*
static std::string to_str(std::vector<markup_t> const &v) {
  std::string ret;
  const char *strs[] = {"P",   "UL", "OL", "LI",  "TABLE", "INDENTED_PRE",
                        "PRE", "H1", "H2", "H3",  "H4",    "H5",
                        "H6",  "B",  "EM", "CODE"};
  for (size_t i = 0; i < v.size(); i++) {
    if (i > 0) {
      ret += " ";
    }
    ret += strs[int(v[i])];
  }
  return ret;
}
*/

// ----------------------------------------------------------------------------

namespace { // put status_t into anonymous namespace to avoid linkage problem

struct status_t {

  // general //////////////////////////////////////////////////////////////////

  cursor_t currpos;
  bool inside_comment;
  cursor_t inside_comment_cursor;
  std::string pre_language;
  markdown_infos_t mi;

  void error(std::string const &msg) const {
    NS2_THROW(markdown_parser_error_t, currpos.to_string() + ": " + msg);
  }

  std::string translate_begin(markup_t markup) {
    if (markup == PRE && mi.output_format == HTML && mi.use_highlight_js) {
      if (pre_language.size() == 0) {
        return "<pre class=\"nohighlight\"><code>";
      } else {
        return "<pre class=\"" + pre_language + "\"><code>";
      }
    } else {
      return t_begin[size_t(mi.output_format)][size_t(markup)];
    }
  }

  std::string translate_end(markup_t markup) {
    return t_end[size_t(mi.output_format)][size_t(markup)];
  }

  // inside paragraph (inline) specific ///////////////////////////////////////

  bool inside_b, inside_em, inside_code, inside_raw_a, inside_mathjax;
  int inside_a, inside_img, inside_header;

  cursor_t inside_b_cursor, inside_em_cursor, inside_code_cursor,
      inside_mathjax_cursor, inside_raw_a_cursor, inside_a_cursor,
      inside_img_cursor, pre_cursor;

  std::string internal_buffer, raw_buffer;
  std::string *external_buffer;
  std::string *out_buffer;

  std::string br() {
    switch (mi.output_format) {
    case HTML:
      return "<br>";
    }
    return ""; // To handle GCC warning
  }

  std::string href(std::string const &url, std::string const &lbl) {
    switch (mi.output_format) {
    case HTML:
      return html_href(url, lbl);
    }
    return ""; // To handle GCC warning
  }

  std::string img(std::string const &url, std::string const &lbl) {
    switch (mi.output_format) {
    case HTML:
      return html_img(url, lbl);
    }
    return ""; // To handle GCC warning
  }

  std::string thead(std::vector<std::string> const &cells) {
    switch (mi.output_format) {
    case HTML:
      return "<thead>\n" + html_th_with_alignments(cells, table_alignment) +
             "\n</thead><tbody>\n";
    }
    return ""; // To handle GCC warning
  }

  std::string tbody_td(std::vector<std::string> const &cells) {
    switch (mi.output_format) {
    case HTML:
      return html_td_with_alignments(cells, table_alignment);
    }
    return ""; // To handle GCC warning
  }

  void bufferize(std::string const &s) {
    if (out_buffer == NULL) {
      out_buffer = external_buffer;
    }
    *out_buffer += s;
  }

  void bufferize_in_raw() { out_buffer = &raw_buffer; }
  void bufferize_in_internal() { out_buffer = &internal_buffer; }
  void bufferize_in_external() { out_buffer = external_buffer; }
  void set_external_buffer(std::string *ptr) { external_buffer = ptr; }

  std::string get_clear_raw_buffer() {
    std::string ret(raw_buffer);
    raw_buffer.clear();
    return ret;
  }

  std::string get_clear_internal_buffer() {
    std::string ret(internal_buffer);
    internal_buffer.clear();
    return ret;
  }

  void error_if_indented_not_closed() {
    if (last_markup_is(PRE)) {
      error("unclosed code block started at " + pre_cursor.to_string());
    }
  }

  void error_if_inlined_not_closed() {
    if (inside_raw_a) {
      error("unfinished link address started at " +
            inside_raw_a_cursor.to_string());
    }
    if (inside_code) {
      error("unclosed inline code started at " +
            inside_code_cursor.to_string());
    }
    if (inside_mathjax) {
      error("unclosed Mathjax code started at " +
            inside_mathjax_cursor.to_string());
    }
    if (inside_em) {
      error("unclosed emphasize started at " + inside_em_cursor.to_string());
    }
    if (inside_b) {
      error("unclosed bold started at " + inside_b_cursor.to_string());
    }
    if (inside_img == 1) {
      error("unfinished image url started at " +
            inside_img_cursor.to_string());
    }
    if (inside_img == 2) {
      error("unfinished image alt started at " +
            inside_img_cursor.to_string());
    }
    if (inside_a == 1) {
      error("unfinished link url started at " + inside_img_cursor.to_string());
    }
    if (inside_a == 2) {
      error("unfinished link text started at " +
            inside_img_cursor.to_string());
    }
  }

  // paragraph (block) specific ///////////////////////////////////////////////

  int last_indent, inside_pre_nb_nl, inside_table;
  bool outside_par, first_line_of_pre, first_line_of_p;
  std::vector<markup_t> markups, indented_markups;
  std::vector<std::string> table_header;
  std::vector<text_align_t> table_alignment;

  // ---

  std::string begin_markup(markup_t const &markup) {
    markups.push_back(markup);
    if (is_indented_markup(markup)) {
      indented_markups.push_back(markup);
    }
    if (markup == PRE || markup == INDENTED_PRE) {
      first_line_of_pre = true;
      outside_par = false;
    }
    if (markup == P) {
      outside_par = false;
      first_line_of_p = true;
    }
    return translate_begin(markup);
  }

  // ---

  bool last_markup_is(markup_t const &markup) const {
    if (markups.size() == 0) {
      return false;
    }
    return markups.back() == markup;
  }

  // ---

  std::string end_markup() {
    assert(markups.size() > 0);
    std::string ret(translate_end(markups.back()));
    if (markups.back() == P || markups.back() == INDENTED_PRE ||
        markups.back() == PRE) {
      outside_par = true;
    }
    // if markup to be poped out is a P, or a PRE or an INDENTED_PRE and if
    // it is not inside a LI then we add a newline
    if ((markups.back() == P || markups.back() == PRE ||
         markups.back() == INDENTED_PRE) &&
        (markups.size() == 1 || markups[markups.size() - 2] != LI)) {
      ret += "\n";
    }
    if (is_indented_markup(markups.back()) || markups.back() == P) {
      error_if_inlined_not_closed();
    }
    if (is_indented_markup(markups.back())) {
      indented_markups.pop_back();
    }
    markups.pop_back();
    return ret;
  }

  // ---

  std::string end_indented_markup(size_t nb = 1) {
    // std::cerr << "DEBUG: before = " << nb << std::endl;
    // std::cerr << "DEBUG: before = " << to_str(status.markups) << std::endl;
    std::string ret;
    size_t i = 0;
    while (markups.size() > 0 && i < nb) {
      if (is_indented_markup(markups.back())) {
        i++;
      }
      ret += end_markup();
    }
    // std::cerr << "DEBUG:  after = " << to_str(status.markups) << std::endl;
    // std::cerr << "---" << std::endl;
    // std::cerr << ret << std::endl;
    // std::cerr << "--------" << std::endl;
    return ret;
  }

  // ---

  std::string end_p_or_table() {
    if (markups.size() == 0) {
      return std::string();
    }
    if (markups.back() != P && markups.back() != TABLE) {
      return std::string();
    }

    error_if_inlined_not_closed();
    inside_table = 0;

    // std::cout << "DEBUG: end_p_or_table: " << to_str(markups) << std::endl;
    // std::cout << "DEBUG: "
    //          << (markups.size() == 0 ||
    //              (markups.back() != P && markups.back() != TABLE))
    //          << std::endl;

    if (markups.size() == 0) {
      return std::string();
    }
    // if (markups.back() != P && markups.back() != TABLE) {
    //  error("internal error #" + std::to_string(__LINE__));
    //}
    std::string ret(end_markup());
    // std::cout << "DEBUG: end_p_or_table: " << ret << std::endl;
    // std::cout << "DEBUG:   inside_table = " << inside_table << std::endl;
    return ret;
  }

  // ---

  std::string end_all_markups_to(markup_t const &markup) {
    std::string ret;
    while (markups.size() > 0 && markups.back() != markup) {
      ret += end_markup();
    }
    if (markups.size() > 0) {
      ret += end_markup();
    }
    return ret;
  }

  // ---

  std::string end_all_markups() {
    std::string ret;
    while (markups.size()) {
      ret += end_markup();
    }
    return ret;
  }

  // ---

  bool inside_pre() {
    // std::cout << "DEBUG: inside_pre = "
    //          << (markups.size() > 0 &&
    //              (markups.back() == PRE || markups.back() == INDENTED_PRE))
    //          << std::endl;
    return markups.size() > 0 &&
           (markups.back() == PRE || markups.back() == INDENTED_PRE);
  }

  // ctor /////////////////////////////////////////////////////////////////////

  void init(markdown_infos_t const &mi_) {
    mi = mi_;
    last_indent = 0;
    inside_pre_nb_nl = 0;
    inside_table = 0;
    outside_par = true;
    first_line_of_pre = false;
    first_line_of_p = false;
    out_buffer = NULL;
    inside_b = false;
    inside_em = false;
    inside_code = false;
    inside_mathjax = false;
    inside_raw_a = false;
    inside_a = 0;
    inside_img = 0;
    inside_comment = false;
    inside_header = false;
    currpos.lineno = 0;
    currpos.col = 0;
  }

  // default ctor only init block specific stuff
  status_t(markdown_infos_t const &mi_) { init(mi_); }

  // copy ctor with extra arguments:
  // - we clear `out_buffer`
  status_t(status_t const &other, std::string *output) {
    init(other.mi);
    currpos = other.currpos;
    external_buffer = output;
    out_buffer = NULL;
  }
};

} // anonymous namespace

// ----------------------------------------------------------------------------

static int get_raw_indent(std::string const &line) {
  int ret = 0;
  for (size_t i = 0; i < line.size(); i++) {
    if (line[i] == ' ') {
      ret += 1;
    } else if (line[i] == '\t') {
      ret += 8;
    } else {
      break;
    }
  }
  return ret;
}

// ----------------------------------------------------------------------------

static bool is_ul_beginning(std::string const &line, size_t i0) {
  if (i0 + 1 >= line.size()) {
    return false;
  }
  return (line[i0] == '-' || line[i0] == '*' || line[i0] == '+') &&
         line[i0 + 1] == ' ';
}

// ---

static bool is_ol_beginning(std::string const &line, size_t i0) {
  if (i0 + 2 >= line.size()) {
    return false;
  }
  return (line[i0] >= '0' && line[i0] <= '9') && line[i0 + 1] == '.' &&
         line[i0 + 2] == ' ';
}

// ---

static bool is_macro_beginning(std::string const &line, size_t i0) {
  if (i0 + 1 >= line.size()) {
    return false;
  }
  return line[i0] == '@' && line[i0 + 1] == '[';
}

// ---

static bool is_pre_beginning(std::string const &line, size_t i0) {
  // std::cout << "DEBUG: line = >" << line << "< | i0 = " << i0 << " | return
  // "
  //          << (line[i0] == '`' && line[i0 + 1] == '`' && line[i0 + 2] ==
  //          '`')
  //          << std::endl;
  if (i0 + 3 > line.size()) {
    return false;
  }
  return line[i0] == '`' && line[i0 + 1] == '`' && line[i0 + 2] == '`';
}

// ---

static bool is_table_row(status_t const &status, std::string const &line,
                         size_t i0) {
  // when inside code or mathjax, pipes are not part of a table
  bool inside_code = status.inside_code;
  bool inside_mathjax = status.inside_mathjax;
  for (size_t i = i0; i < line.size(); i++) {
    if (line[i] == '\\' && (i + 1) < line.size() && line[i + 1] == '|') {
      i++;
      continue;
    }
    if (line[i] == '`') {
      inside_code = !inside_code;
    }
    if (line[i] == '$') {
      inside_mathjax = !inside_mathjax;
    }
    if (line[i] == '|' && !inside_mathjax && !inside_code) {
      return true;
    }
  }
  return false;
}

// ---

// necessary forward declaration
static void translate_para(status_t *, std::string const &, std::string *);

// this function assumes that the first and last (if any) pipe are removed
static std::vector<std::pair<size_t, std::string> >
split_table_row(status_t const &status, std::string const &line_) {
  std::vector<size_t> cells_i0;
  std::vector<std::pair<size_t, std::string> > cells;
  std::string cell;
  size_t cell_beginning = 1;
  size_t col_offset = status.currpos.col; // we get cursor column

  // ignore leading and add ending pipe if not present (it is easier to parse)
  size_t begin;
  for (begin = 0;
       begin < line_.size() && (line_[begin] == ' ' || line_[begin] == '\t');
       begin++)
    ;
  if (line_[begin] == '|') {
    begin++;
  }
  size_t end = line_.size() - 1;
  for (; line_[end] == ' ' || line_[end] == '\t'; end--)
    ;
  if (line_[end] == '|') {
    end--;
  }
  std::string line(line_.substr(begin, end - begin + 1) + "|");
  col_offset += begin; // we add begin to cursor column

  // split row into cells
  for (size_t i = 0; i < line.size(); i++) {
    if (line[i] == '\\' && (i + 1) < line.size() && line[i + 1] == '|') {
      cell_beginning = (cell_beginning == size_t(-1) ? i : cell_beginning);
      cell += '|';
      i++;
      continue;
    } else if (line[i] == '|') {
      // std::cout << "DEBUG: new cell = >" << cell << "<" << std::endl;
      cell_beginning = (cell_beginning == size_t(-1) ? i - 1 : cell_beginning);
      cells.push_back(std::pair<size_t, std::string>(
          cell_beginning + col_offset, ns2::strip(cell)));
      cell.clear();
      cell_beginning = size_t(-1);
      continue;
    } else {
      cell_beginning = (cell_beginning == size_t(-1) ? i : cell_beginning);
      cell += line[i];
      continue;
    }
  }
  // std::cout << "DEBUG: new cell = >" << cell << "<" << std::endl;
  return cells;
}

// ---

static std::vector<std::string> translate_table_row(std::string const &line,
                                                    status_t *status_) {
  status_t &status = *status_;
  std::vector<std::pair<size_t, std::string> > cells =
      split_table_row(status, line);
  std::vector<std::string> ret;
  for (size_t i = 0; i < cells.size(); i++) {
    ret.push_back(std::string());
    status_t cell_status(status, &(ret.back()));
    cell_status.currpos.col = cells[i].first;
    translate_para(&cell_status, cells[i].second, &(ret.back()));
    cell_status.error_if_inlined_not_closed();
  }
  return ret;
}

// ---

static std::vector<text_align_t> get_table_alignment(status_t *status_,
                                                     std::string const &line) {
  status_t &status = *status_;
  std::vector<std::pair<size_t, std::string> > cells =
      split_table_row(status, line);
  std::vector<text_align_t> ret;
  for (size_t i = 0; i < cells.size(); i++) {
    // std::cout << "DEBUG: " << cells[i] << std::endl;
    status.currpos += 1;
    std::string buf(ns2::strip(cells[i].second));
    if (buf.size() < 3) {
      status.error("delimiter row is ill-formed");
    }
    if (buf == std::string(buf.size(), '-') ||
        (buf[0] == ':' && buf.substr(1) == std::string(buf.size() - 1, '-'))) {
      ret.push_back(Left);
    } else if (buf[0] == ':' && buf[buf.size() - 1] == ':' &&
               buf.substr(1, buf.size() - 2) ==
                   std::string(buf.size() - 2, '-')) {
      ret.push_back(Center);
    } else if (buf[buf.size() - 1] == ':' &&
               buf.substr(0, buf.size() - 1) ==
                   std::string(buf.size() - 1, '-')) {
      ret.push_back(Right);
    } else {
      status.error("delimiter row is ill-formed");
    }
    status.currpos += buf.size();
  }
  return ret;
}

// ---

static void inside_pre_prepend_nl(status_t *status_,
                                  std::string *rest_of_the_line) {
  status_t &status = *status_;

  if (!status.inside_pre()) {
    return;
  }
  if (status.first_line_of_pre) {
    status.inside_pre_nb_nl = 0;
    if (rest_of_the_line->size() > 0) {
      status.first_line_of_pre = false;
    }
  } else {
    if (rest_of_the_line->size() == 0) {
      status.inside_pre_nb_nl++;
    } else {
      rest_of_the_line->insert(0, size_t(status.inside_pre_nb_nl) + 1, '\n');
      status.inside_pre_nb_nl = 0;
    }
  }
}

// ---

static void inside_p_prepend_nl(status_t *status_,
                                std::string *rest_of_the_line) {
  status_t &status = *status_;

  if (!status.first_line_of_p) {
    rest_of_the_line->insert(0, 1, '\n');
  }
  status.first_line_of_p = false;
}

// ----------------------------------------------------------------------------

static std::string do_macro(status_t const &status, std::string const &line,
                            size_t i0) {
  // i0 must point to '@['
  i0 += 2; // now i0 points to the char after '@['
  size_t i1 = line.find("](");
  if (i1 == std::string::npos) {
    status.error("unable to find end of first argument");
  }
  std::string arg1(line.substr(i0, i1 - i0));
  i1 += 2; // i1 points to the char after ']('
  size_t i2 = line.rfind(")");
  if (i2 == std::string::npos) {
    status.error("unable to find end of second argument");
  }
  std::string arg2(line.substr(i1, i2 - i1));
  return status.mi.callback_macro(arg1, arg2, status.mi) + "\n";
}

// ----------------------------------------------------------------------------

static void close_open_indented_markups(status_t *status_,
                                        std::string const &line,
                                        std::string *rest_of_the_line_,
                                        std::string *markups_close_open) {
  status_t &status = *status_;
  std::string &rest_of_the_line = *rest_of_the_line_;
  markups_close_open->clear();

  // std::cerr << "DEBUG: line = >" << line << "<" << std::endl;
  // std::cout << "DEBUG: current markups = " << to_str(status.markups)
  //           << std::endl;

  // close any header
  if (status.inside_header) {
    status.error_if_inlined_not_closed();
    *markups_close_open = status.end_all_markups();
    status.inside_header = false;
  }

  // we first compute at which level of indentation we are
  // raw_indent = real raw indentation = number of spaces at beginning of line
  // curr_indent = number of opnened indented markup we are
  // curr_raw_indent = number of spaces corresponding to the number of opened
  //                   indented markups
  size_t raw_indent = size_t(get_raw_indent(line));
  size_t curr_indent = 0;
  size_t curr_raw_indent = 0;
  int temp = int(raw_indent);
  for (size_t i = 0; i < status.indented_markups.size(); i++) {
    int markup_indent = markup_indents[size_t(status.indented_markups[i])];
    if (temp - markup_indent < 0) {
      break;
    }
    temp -= markup_indent;
    curr_raw_indent += size_t(markup_indent);
    curr_indent++;
  }

  // are we inside a PRE
  if (status.last_markup_is(PRE)) {
    if (std::string(line, curr_raw_indent) == "```") {
      *markups_close_open += status.end_markup();
      rest_of_the_line.clear();
    } else {
      rest_of_the_line = line.substr(curr_raw_indent);
      status.currpos += curr_raw_indent;
      inside_pre_prepend_nl(&status, &rest_of_the_line);
    }
    return;
  }

  // if we are within an INDENTED_PRE and that either
  // - the stripped line is blank
  // - or the indentation level has not dropped
  // then we just pass the rest of the line as-is
  if (status.last_markup_is(INDENTED_PRE) &&
      (ns2::strip(line).size() == 0 ||
       status.indented_markups.size() == curr_indent)) {
    rest_of_the_line = std::string(line, curr_raw_indent);
    status.currpos += curr_raw_indent;
    inside_pre_prepend_nl(&status, &rest_of_the_line);
    return;
  }

  // empty line has an effect only when not in an INDENTED_PRE
  if (ns2::strip(line).size() == 0) {

    //std::cout << "DEBUG: empty line" << std::endl;
    //std::cout << "DEBUG:   markups before = " << to_str(status.markups) <<
    //              std::endl;

    // error check when inside tables
    if (status.last_markup_is(TABLE)) {
      if (status.inside_table == 1) {
        status.error("table is incomplete, delimiter row missing");
      }
      if (status.inside_table == 2) {
        status.error("table is incomplete, table body missing");
      }
    }
    *markups_close_open += status.end_p_or_table();
    status.outside_par = true;
    rest_of_the_line.clear();

    //std::cout << "DEBUG:   markups after  = " << to_str(status.markups) <<
    //              std::endl;

    return;
  }

  // here temp must be 0 or >= 4 otherwise indentation is ill-formed
  if (temp > 0 && temp < 4) {
    status.error("indentation is ill-formed");
    return;
  }

  // if string begins with some #'s then it is a title
  size_t nb_sharp = 0;
  for (nb_sharp = 0; nb_sharp < line.size() && line[nb_sharp] == '#';
       nb_sharp++)
    ;

  if (nb_sharp > 0 && nb_sharp <= 6 && line[nb_sharp] == ' ') {
    // std::cout << "DEBUG:   header #" << nb_sharp << std::endl;
    *markups_close_open += status.end_all_markups();
    *markups_close_open += status.begin_markup(markup_t(H1 + (nb_sharp - 1)));
    rest_of_the_line = line.substr(nb_sharp + 1);
    status.currpos += nb_sharp + 1;
    status.inside_header = true;
    return;
  }

  // lower indentation level
  if (curr_indent < status.indented_markups.size()) {
    // std::cout << "DEBUG:   lower indentation level" << std::endl;

    // close (status.indented_markups.size() - curr_indent - 1) indented
    // markups
    *markups_close_open += status.end_indented_markup(
        status.indented_markups.size() - curr_indent - 1);

    // are we in the middle of an unordered list
    if (is_ul_beginning(line, raw_indent)) {
      switch (status.indented_markups.back()) {
      case OL:
        status.error("cannot switch to unordered list");
        return;
      case UL:
        *markups_close_open += status.end_all_markups_to(LI);
        *markups_close_open += status.begin_markup(LI);
        *markups_close_open += status.begin_markup(P);
        rest_of_the_line = line.substr(raw_indent + 2);
        status.currpos += raw_indent + 2;
        inside_p_prepend_nl(&status, &rest_of_the_line);
        return;
      default:
        *markups_close_open += status.end_indented_markup(1);
        *markups_close_open += status.begin_markup(UL);
        *markups_close_open += status.begin_markup(LI);
        *markups_close_open += status.begin_markup(P);
        inside_p_prepend_nl(&status, &rest_of_the_line);
        return;
      }
    }

    // are we in the middle of an ordered list
    if (is_ol_beginning(line, raw_indent)) {
      switch (status.indented_markups.back()) {
      case UL:
        status.error("cannot switch to ordered list");
        return;
      case OL:
        *markups_close_open += status.end_all_markups_to(LI);
        *markups_close_open += status.begin_markup(LI);
        *markups_close_open += status.begin_markup(P);
        rest_of_the_line = line.substr(raw_indent + 3);
        status.currpos += raw_indent + 3;
        inside_p_prepend_nl(&status, &rest_of_the_line);
        return;
      default:
        *markups_close_open += status.end_indented_markup(1);
        *markups_close_open += status.begin_markup(OL);
        *markups_close_open += status.begin_markup(LI);
        *markups_close_open += status.begin_markup(P);
        inside_p_prepend_nl(&status, &rest_of_the_line);
        return;
      }
    }

    // do we have an unindented pre
    if (is_pre_beginning(line, raw_indent)) {
      status.pre_language = strip(line.substr(raw_indent + 3));
      *markups_close_open += status.end_indented_markup(1);
      *markups_close_open += status.begin_markup(PRE);
      rest_of_the_line.clear();
      status.pre_cursor = status.currpos + size_t(raw_indent);
      return;
    }

    // do we have a macro (only works when we have a callback)
    if (status.mi.callback_macro != NULL &&
        is_macro_beginning(line, raw_indent)) {
      *markups_close_open += status.end_indented_markup(1);
      *markups_close_open += do_macro(status, line, raw_indent);
      rest_of_the_line.clear();
      return;
    }

    // do we have a table row
    if (is_table_row(status, line, raw_indent)) {
      *markups_close_open += status.end_indented_markup(1);
      status.inside_table = 1;
      *markups_close_open += status.begin_markup(TABLE);
      // then strip leading and ending pipe if any and let translate_para act
      rest_of_the_line = line.substr(raw_indent);
      status.currpos += raw_indent;
      return;
    }

    // when not inside lists then close remaining indented markup
    if (status.indented_markups.size() > 0) {
      status.error_if_indented_not_closed();
      *markups_close_open += status.end_indented_markup(1);
    }

    // normal paragraph
    *markups_close_open += status.begin_markup(P);
    rest_of_the_line = line.substr(raw_indent);
    inside_p_prepend_nl(&status, &rest_of_the_line);
    return;
  }

  // same indentation level
  if (status.indented_markups.size() == curr_indent) {
    //std::cout << "DEBUG: same indentation level" << std::endl;

    // do we have a table row
    if (is_table_row(status, line, raw_indent)) {
      if (status.inside_table == 0) {
        *markups_close_open += status.end_p_or_table();
        *markups_close_open += status.begin_markup(TABLE);
      }
      status.inside_table = (std::min)(3, status.inside_table + 1);
      rest_of_the_line = line.substr(raw_indent);
      status.currpos += raw_indent;
      return;
    }

    // start a new unordered list
    if (is_ul_beginning(line, raw_indent)) {
      //std::cout << "DEBUG: new unordered list" << std::endl;
      //std::cout << "DEBUG:   markups before = " << to_str(status.markups) <<
      //           std::endl;
      *markups_close_open += status.end_p_or_table();
      *markups_close_open += status.begin_markup(UL);
      *markups_close_open += status.begin_markup(LI);
      *markups_close_open += status.begin_markup(P);
      rest_of_the_line = line.substr(raw_indent + 2);
      status.currpos += raw_indent + 2;
      inside_p_prepend_nl(&status, &rest_of_the_line);
      //std::cout << "DEBUG:   markups after  = " << to_str(status.markups) <<
      //             std::endl;
      // std::cout << "DEBUG:     first_line_of_p = " << status.first_line_of_p
      //           << std::endl;
      return;
    }

    // start a new ordered list
    if (is_ol_beginning(line, raw_indent)) {
      *markups_close_open += status.end_p_or_table();
      *markups_close_open += status.begin_markup(OL);
      *markups_close_open += status.begin_markup(LI);
      *markups_close_open += status.begin_markup(P);
      rest_of_the_line = line.substr(raw_indent + 3);
      status.currpos += raw_indent + 3;
      inside_p_prepend_nl(&status, &rest_of_the_line);

      // std::cerr << std::endl << "---" << std::endl;
      // std::cerr << "DEBUG:       line = >" << line << "<" << std::endl;
      // std::cerr << "DEBUG: raw_indent = " << raw_indent << std::endl;
      // std::cerr << "DEBUG:       rest = >" << rest_of_the_line << "<"
      //           << std::endl;
      // std::cerr << "---" << std::endl;

      return;
    }

    // do we have a macro (only works wen we have a callback)
    if (status.mi.callback_macro != NULL &&
        is_macro_beginning(line, raw_indent)) {
      *markups_close_open += status.end_p_or_table();
      *markups_close_open += do_macro(status, line, raw_indent);
      rest_of_the_line.clear();
      return;
    }

    // start a new unindented code listing
    // std::cout << "DEBUG:   just before verifying is_pre_beginning"
    //           << std::endl;
    if (is_pre_beginning(line, raw_indent)) {
      // std::cout << "DEBUG: is_pre_beginning: " << to_str(status.markups)
      //           << std::endl;
      status.pre_language = strip(line.substr(raw_indent + 3));
      *markups_close_open += status.end_p_or_table();
      // std::cout << "DEBUG:   is_pre_beginning" << std::endl;
      *markups_close_open += status.begin_markup(PRE);
      // std::cout << "DEBUG: is_pre_beginning: " << *markups_close_open
      //           << std::endl;
      rest_of_the_line.clear();
      status.pre_cursor = status.currpos + size_t(raw_indent);
      inside_pre_prepend_nl(&status, &rest_of_the_line);
      return;
    }

    // if we have at least 4 spaces then we begin a new indented code listing
    if (temp >= 4) {
      *markups_close_open += status.end_p_or_table();
      *markups_close_open += status.begin_markup(INDENTED_PRE);
      rest_of_the_line = line.substr(curr_raw_indent + 4);
      status.currpos += raw_indent + 4;
      inside_pre_prepend_nl(&status, &rest_of_the_line);
      return;
    }

    // getting here means that we continue the current paragraph, except if
    // last line was empty
    if (status.outside_par) {
      *markups_close_open += status.begin_markup(P);
    }
    rest_of_the_line = line.substr(raw_indent);
    status.currpos += raw_indent;
    // std::cout << "DEBUG: para = >" << line << "<" << std::endl;
    // std::cout << "DEBUG:   para, before > " << rest_of_the_line << "<"
    //          << std::endl;
    inside_p_prepend_nl(&status, &rest_of_the_line);
    // std::cout << "DEBUG:   para,  after > " << rest_of_the_line << "<"
    //          << std::endl;
    return;
  }
}

// ----------------------------------------------------------------------------

static void translate_para(status_t *status_, std::string const &line,
                           std::string *out_) {
  status_t &status = *status_;
  std::string &out = *out_;
  out.clear();
  status.set_external_buffer(&out);

  // std::cout << "DEBUG: inside_table = " << status.inside_table << std::endl;

  // inside table has a special treatment
  if (status.inside_table == 1) {
    // table header
    status.table_header = translate_table_row(line, &status);
    return;
  } else if (status.inside_table == 2) {
    // row delimiter
    status.table_alignment = get_table_alignment(&status, line);
    if (status.table_alignment.size() != status.table_header.size()) {
      status.error("row delimiters has " +
                   std::to_string(status.table_alignment.size()) +
                   " columns but expected " +
                   std::to_string(status.table_header.size()));
    }
    status.bufferize(status.thead(status.table_header));
    return;
  } else if (status.inside_table == 3) {
    std::vector<std::string> row(translate_table_row(line, &status));
    // for (size_t i = 0; i < row.size(); i++) {
    //  std::cout << "DEBUG: row[" << i << "] = >" << row[i] << "<" <<
    //  std::endl;
    //}
    if (row.size() != status.table_header.size()) {
      status.error("row has " + std::to_string(row.size()) +
                   " columns but expected " +
                   std::to_string(status.table_header.size()));
    }
    status.bufferize(status.tbody_td(row) + "\n");
    return;
  }

  for (size_t i = 0; i < line.size(); i++) {
    status.currpos.nextchar();

    //std::cout << "DEBUG: current char = >" << line[i] << "<" << std::endl;

    if (status.last_markup_is(PRE) || status.last_markup_is(INDENTED_PRE)) {

      // inside a PRE or INDENTED_PRE, we just dump characters
      status.bufferize(htmlize(line[i]));
      continue;

    } else if (status.inside_code || status.inside_mathjax ||
               status.inside_raw_a || status.inside_a == 2 ||
               status.inside_img == 2) {

      // end inline code
      if (status.inside_code && line[i] == '`') {
        status.bufferize(status.translate_end(CODE));
        status.inside_code = false;
        continue;
      }

      // end Mathjax code
      if (status.inside_mathjax && line[i] == '$') {
        status.bufferize(status.translate_end(MATHJAX));
        status.inside_mathjax = false;
        continue;
      }

      // end of link
      if ((status.inside_raw_a && line[i] == '>') ||
          (status.inside_a == 2 && line[i] == ')')) {
        std::string url(status.get_clear_raw_buffer());
        std::string lbl(status.inside_raw_a && line[i] == '>'
                            ? htmlize(url)
                            : status.get_clear_internal_buffer());
        std::pair<std::string, bool> res(std::string(), false);
        if (status.mi.callback_link != NULL) {
          res = status.mi.callback_link(lbl, url, status.mi);
        }
        status.bufferize_in_external();
        if (res.second) {
          status.bufferize(res.first);
        } else {
          status.bufferize(status.href(url, lbl));
        }
        status.inside_raw_a = false;
        status.inside_a = 0;
        continue;
      }

      // end of img
      if (status.inside_img == 2 && line[i] == ')') {
        status.bufferize_in_external();
        status.bufferize(status.img(status.get_clear_raw_buffer(),
                                    status.get_clear_internal_buffer()));
        status.inside_img = 0;
        continue;
      }

    } else {

      // backslash
      if (line[i] == '\\') {
        if (i == line.size() - 1) {
          status.bufferize(status.br());
        } else {
          status.bufferize(htmlize(line[i + 1]));
        }
        i++;
        status.currpos.advanceby(1);
        continue;
      }

      // ** strong emphasize or bold
      if (i + 1 < line.size() && line[i] == '*' && line[i + 1] == '*') {
        // std::cout << "DEBUG: strong emphasize" << std::endl;

        if (!status.inside_b) {
          status.bufferize(status.translate_begin(B));
        } else {
          status.bufferize(status.translate_end(B));
        }
        status.inside_b = !status.inside_b;
        status.inside_b_cursor = status.currpos;
        i++;
        status.currpos.advanceby(1);
        continue;
      }

      // * emphasis
      if (line[i] == '*') {
        if (!status.inside_em) {
          status.bufferize(status.translate_begin(EM));
        } else {
          status.bufferize(status.translate_end(EM));
        }
        status.inside_em = !status.inside_em;
        status.inside_em_cursor = status.currpos;
        continue;
      }

      // ` inline code
      if (line[i] == '`' && !status.inside_code) {
        status.bufferize(status.translate_begin(CODE));
        status.inside_code_cursor = status.currpos;
        status.inside_code = true;
        continue;
      }

      // $ Mathjax code
      if (line[i] == '$' && !status.inside_mathjax) {
        status.bufferize(status.translate_begin(MATHJAX));
        status.inside_mathjax_cursor = status.currpos;
        status.inside_mathjax = true;
        continue;
      }

      // < raw link beginning
      if (i + 1 < line.size() && line[i] == '<' && line[i + 1] != ' ') {
        status.bufferize_in_raw();
        status.inside_raw_a = true;
        status.inside_raw_a_cursor = status.currpos;
        continue;
      }

      // [ link begining
      if (line[i] == '[') {
        status.bufferize_in_internal();
        status.inside_a_cursor = status.currpos;
        status.inside_a = 1;
        continue;
      }

      // ![ image begining
      if (i + 1 < line.size() && line[i] == '!' && line[i + 1] == '[') {
        status.bufferize_in_internal();
        status.inside_img = 1;
        status.inside_img_cursor = status.currpos;
        i++;
        status.currpos.advanceby(1);
        continue;
      }
      // ]( separator between text and url
      if (i + 1 < line.size() && line[i] == ']' && line[i + 1] == '(') {
        if (status.inside_a == 1) {
          status.inside_a = 2;
          status.inside_a_cursor = status.currpos;
        }
        if (status.inside_img == 1) {
          status.inside_img = 2;
          status.inside_img_cursor = status.currpos;
        }
        status.bufferize_in_raw();
        i++;
        status.currpos.advanceby(1);
        continue;
      }

      // ] alone we just change buffer to external
      if (line[i] == ']' && (i == line.size() - 1 || line[i + 1] != '(')) {
        status.inside_a = 0;
        status.inside_img = 0;
        status.bufferize_in_external();
        status.bufferize("[" + status.get_clear_internal_buffer() + "]");
        continue;
      }
    }

    // last 2 spaces means force line break
    if (i == line.size() - 2 && line[i] == ' ' && line[i + 1] == ' ') {
      status.bufferize(status.br());
      break;
    } else {
      // finally a simple character
      //std::cout << "DEBUG: bufferize(" << line[i] << ")" << std::endl;
      status.bufferize(htmlize(line[i]));
    }
  }
}

// ----------------------------------------------------------------------------

NS_DLLSPEC void compile_markdown(std::istream *in_, std::ostream *out_,
                                 markdown_infos_t const &markdown_infos) {
  std::istream &in = *in_;
  std::ostream &out = *out_;
  std::string line;
  status_t status(markdown_infos);

  while (!in.eof()) {
    // read line
    std::getline(in, line);
    if (line.size() > 0 && line.back() == '\r') { // Windows endline...
      line.pop_back();
    }
    status.currpos.newline();
    status.currpos.line = line;

    // std::cout << std::endl;
    // std::cout << "DEBUG: line = >" << line << "<" << std::endl;

    // check for comments
    // first remove all "<!-- ... -->" on the same line
    std::string buf;
    bool comment_in_line = false;
    for (size_t i = 0; i < line.size(); i++) {
      if (i + 3 < line.size() && line.substr(i, 4) == "<!--") {
        if (!status.inside_comment) {
          comment_in_line = true;
          status.inside_comment = true;
          status.inside_comment_cursor = status.currpos + i;
        }
        i += 3;
        continue;
      }
      if (i + 2 < line.size() && line.substr(i, 3) == "-->") {
        if (!status.inside_comment) {
          buf += "-->";
        } else {
          comment_in_line = true;
          status.inside_comment = false;
        }
        i += 2;
        continue;
      }
      if (!status.inside_comment) {
        buf += line[i];
      }
    }
    line = buf;

    // std::cout << "DEBUG: after line = >" << line << "<" << std::endl;
    // std::cout << "DEBUG: comment_in_line = " << comment_in_line <<
    // std::endl;

    // empty lines inside comments are ignored
    if (line.size() == 0 && (comment_in_line || status.inside_comment)) {
      // std::cout << "DEBUG: line is empty and commented: skipping" <<
      // std::endl;
      continue;
    }

    // close/open blocks
    std::string rest_of_the_line;
    std::string output;
    close_open_indented_markups(&status, line, &rest_of_the_line, &output);
    out << output;
    // std::cout << "DEBUG: after markups: output = " << output << std::endl;
    // std::cout << "DEBUG: after markups:   rest = " << rest_of_the_line
    //           << std::endl;

    // translate rest of the line
    translate_para(&status, rest_of_the_line, &output);
    out << output;
    // std::cout << "DEBUG: after translate: " << output << std::endl;
  }

  // check for non closed indented markups
  status.error_if_indented_not_closed();

  // check for non closed comment
  if (status.inside_comment) {
    status.error("unclosed comment started at " +
                 status.inside_comment_cursor.to_string());
  }

  out << status.end_all_markups() << std::flush;
}

// ----------------------------------------------------------------------------

NS_DLLSPEC void compile_markdown(std::string const &in_, std::string *out_,
                                 markdown_infos_t const &markdown_infos) {
  std::istringstream in(in_);
  out_->clear();
  std::ostringstream out(*out_);
  compile_markdown(&in, &out, markdown_infos);
  *out_ = out.str();
}

// ----------------------------------------------------------------------------

} // namespace ns2

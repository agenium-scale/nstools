#include <ns2.hpp>

#include <string>
#include <vector>
#include <istream>
#include <ctime>

// ----------------------------------------------------------------------------

struct token_t {
  std::string text;
  ns2::cursor_t cursor;
  std::string cpp_text;
};

// ----------------------------------------------------------------------------

static bool is_blank(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ----------------------------------------------------------------------------

static bool is_token(char c) {
  if (c == ',' || c == '{' || c == '}' || c == ':') {
    return true;
  } else {
    return false;
  }
}

// ----------------------------------------------------------------------------

static bool is_token(std::string const &line, size_t i0,
                     std::string const &token_text) {
  if (i0 + token_text.size() - 1 >= line.size()) {
    return false;
  }
  if (std::string(line, i0, token_text.size()) != token_text) {
    return false;
  }
  size_t after = i0 + token_text.size();
  if (after >= line.size()) {
    return true;
  }
  if (is_blank(line[after]) || is_token(line[after]) || line[after] == '"') {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------

static void tokenize(std::vector<token_t> *tokens_, std::istream *in_,
                     std::string *error_) {
  std::istream &in = *in_;
  std::string &error = *error_;
  ns2::cursor_t currpos;
  std::vector<token_t> &tokens = *tokens_;

  while (!in.eof()) {
    std::string line;
    std::getline(in, line);
    if (line.size() > 0 && line[line.size() - 1] == '\r') { // Windows endline
      line.resize(line.size() - 1);
    }
    currpos.newline();
    currpos.line = line;

    for (size_t i = 0; i < line.size(); i++) {

      currpos.nextchar();
      if (is_blank(line[i])) {
        continue;
      }

      // Comments
      if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') {
        break;
      }

      // Strings
      if (line[i] == '"') {
        size_t j = line.find('"', i + 1);
        if (j == std::string::npos) {
          error = "unterminated string began at " + currpos.to_string();
          return;
        }
        token_t token;
        token.text = std::string(line, i, j - i + 1);
        token.cursor = currpos;
        tokens.push_back(token);
        currpos.advanceby(j - i);
        i = j;
        continue;
      }

      // Single char tokens such as coma, left and right curly bracket
      if (is_token(line[i])) {
        token_t token;
        token.text = std::string(line, i, 1);
        token.cursor = currpos;
        tokens.push_back(token);
        continue;
      }

#define HELPER(token_text)                                                    \
  if (is_token(line, i, token_text)) {                                        \
    token_t token;                                                            \
    token.text = std::string(token_text);                                     \
    token.cursor = currpos;                                                   \
    tokens.push_back(token);                                                  \
    i += sizeof(token_text) - 2;                                              \
    currpos.advanceby(sizeof(token_text) - 2);                                \
    continue;                                                                 \
  }

      HELPER("or")
      HELPER("double")
      HELPER("vector_double")
      HELPER("string")
      HELPER("vector_string")
      HELPER("bool")
      HELPER("vector_bool")

#undef HELPER

      error = "unknown token at " + currpos.to_string();
      return;

    }
  }
}

// ----------------------------------------------------------------------------

struct tree_t {
  // For checking only
  std::set<std::string> chk_keys;
  std::map<std::string, token_t> chk_key_to_token;

  // Could have used std::unique_ptr to avoid writing the dtor, but it is
  // C++11 and we are C++98.
  struct entry_t {
    std::vector<token_t> keys;
    token_t type;
    tree_t *subtree;
  };
  std::vector<entry_t> data;

  ~tree_t() {
    for (size_t i = 0; i < data.size(); i++) {
      if (data[i].subtree != NULL) {
        delete data[i].subtree;
      }
    }
  }

  void find(std::vector<token_t> const &keys, std::string *error_) const {
    std::string &error = *error_;
    for (size_t i = 0; i < keys.size(); i++) {
      if (chk_keys.find(keys[i].text) != chk_keys.end()) {
        ns2::cursor_t const &cursor = chk_key_to_token.at(keys[i].text).cursor;
        error = "key already exists at this level: " + cursor.to_string();
        return;
      }
    }
  }

  void append(std::vector<token_t> const &keys, token_t const &type,
              tree_t *subtree, std::string *error_) {
    std::string &error = *error_;

    find(keys, &error);
    if (error.size() > 0) {
      return;
    }

    // update checking data structures
    for (size_t i = 0; i < keys.size(); i++) {
      chk_keys.insert(keys[i].text);
      chk_key_to_token.insert(
          std::pair<std::string, token_t>(keys[i].text, keys[i]));
    }

    // Finally add the keys
    entry_t entry;
    entry.keys = keys;
    entry.type = type;
    entry.subtree = subtree;
    data.push_back(entry);
  }
};

// ----------------------------------------------------------------------------

static void parse_rec(tree_t *tree_, std::string *error_,
                      std::vector<token_t> const &tokens, size_t *i_) {
  std::string &error = *error_;
  tree_t &tree = *tree_;
  size_t &i = *i_;

  if (tokens[i].text != "{") {
    error = "expecting '{' here " + tokens[i].cursor.to_string();
    return;
  }

  enum expect_t { Map, Key, OrColon, Type, ComaEnd } expect;

  std::vector<token_t> keys;
  expect = Map;
  for (; i < tokens.size(); i++) {

    switch (expect) {
    case Map:
      if (tokens[i].text != "{") {
        error = "expecting '{' here " + tokens[i].cursor.to_string();
        return;
      }
      expect = Key;
      continue;
    case Key:
      if (tokens[i].text[0] != '"') {
        error = "expecting string as key here " + tokens[i].cursor.to_string();
        return;
      }
      keys.push_back(tokens[i]);
      expect = OrColon;
      continue;
    case OrColon:
      if (tokens[i].text != "or" && tokens[i].text != ":") {
        error = "expecting 'or' or ':' here " + tokens[i].cursor.to_string();
        return;
      }
      if (tokens[i].text == ":") {
        expect = Type;
        continue;
      }
      expect = Key;
      continue;
    case Type:
      if (tokens[i].text == "{") {
        tree_t *subtree = new tree_t;
        tree.append(keys, tokens[i], subtree, &error);
        if (error.size() > 0) {
          return;
        }
        keys.clear();
        parse_rec(subtree, &error, tokens, &i);
        if (error.size() > 0) {
          return;
        }
        expect = ComaEnd;
        continue;
      }
      if (tokens[i].text == "double" || tokens[i].text == "vector_double" ||
          tokens[i].text == "string" || tokens[i].text == "vector_string" ||
          tokens[i].text == "bool" || tokens[i].text == "vector_bool") {
        tree.append(keys, tokens[i], NULL, &error);
        if (error.size() > 0) {
          return;
        }
        keys.clear();
        expect = ComaEnd;
        continue;
      }
      error = "expecting '{', 'double', 'vector_double', 'string', "
              "'vector_string', 'bool' or 'vector_bool' here " +
              tokens[i].cursor.to_string();
      return;
    case ComaEnd:
      if (tokens[i].text == ",") {
        expect = Key;
        continue;
      }
      if (tokens[i].text == "}") {
        return;
      }
      error = "expected '}' or ',' here " + tokens[i].cursor.to_string();
      return;
    }
  }

  error = "unexpected end of file after " +
          tokens[tokens.size() - 1].cursor.to_string() + ", maybe missing '}'";
}

static void parse(tree_t *tree_, std::string *error_,
                  std::vector<token_t> const &tokens) {
  size_t i = 0;
  parse_rec(tree_, error_, tokens, &i);
}

// ----------------------------------------------------------------------------

static void cppize_tokens(std::vector<token_t> *tokens_) {
  std::vector<token_t> &tokens = *tokens_;
  for (size_t i = 0; i < tokens.size(); i++) {
    std::string const &text = tokens[i].text;
    std::string &cpp_text = tokens[i].cpp_text;
    for (size_t j = 0; j < text.size(); j++) {
      if ((text[j] >= 'a' && text[j] <= 'z') ||
          (text[j] >= 'A' && text[j] <= 'Z') ||
          (text[j] >= '0' && text[j] <= '9') || text[j] == '_') {
        cpp_text += text[j];
      } else if (text[j] != '"') {
        cpp_text += "_";
      }
    }
  }
}

// ----------------------------------------------------------------------------

static std::string get_cpp_type(std::string const &type) {
  if (type == "double") {
    return "double";
  } else if (type == "vector_double") {
    return "std::vector<double>";
  } else if (type == "string") {
    return "std::string";
  } else if (type == "vector_string") {
    return "std::vector<std::string>";
  } else if (type == "bool") {
    return "bool";
  } else if (type == "vector_bool") {
    return "std::vector<bool>";
  } else {
    abort(); // should never be reached
  }
}

// ----------------------------------------------------------------------------
// No variadic templates as we are not C++11.
// Variadic number of arguments à la C is not possible with non-PD types.
// So we hack, it is ugly but is works.

static void print(std::ostream *out_, std::string const &fmt,
                  std::string const &a0 = std::string(),
                  std::string const &a1 = std::string(),
                  std::string const &a2 = std::string(),
                  std::string const &a3 = std::string(),
                  std::string const &a4 = std::string(),
                  std::string const &a5 = std::string(),
                  std::string const &a6 = std::string(),
                  std::string const &a7 = std::string(),
                  std::string const &a8 = std::string(),
                  std::string const &a9 = std::string(),
                  std::string const &a10 = std::string(),
                  std::string const &a11 = std::string()) {
  std::ostream &out = *out_;
  std::string const *a[] = {&a0, &a1, &a2, &a3, &a4,  &a5,
                            &a6, &a7, &a8, &a9, &a10, &a11};

  size_t j = 0;
  for (size_t i = 0; i < fmt.size(); i++) {
    if (fmt[i] == '@' && j < 12) {
      out << *(a[j]);
      j++;
    } else if (fmt[i] == '%' && i + 1 < fmt.size() && j < 12) {
      std::string const &s = *(a[j]);
      if (s.size() > 0) {
        out << s;
      } else {
        i++;
      }
      j++;
    } else {
      out << fmt[i];
    }
  }
}

// ----------------------------------------------------------------------------

static void dump_sep(std::ostream *out_) {
  std::ostream &out = *out_;
  out << "\n\n// " << std::string(76, '-') << "\n\n";
}

// ----------------------------------------------------------------------------

static void dump_header(std::ostream *out_, int argc, char **argv) {
  std::ostream &out = *out_;
  char buf[64];
  time_t now = time(NULL);
  if (strftime(buf, 63, "%c", localtime(&now)) > 0) {
    out << "// File automatically generated on " << buf << " by jsonc\n";
  } else {
    out << "// File automatically generated on by jsonc\n";
  }
  out << "// Command line:";
  for (int i = 0; i < argc; i++) {
    out << " " << argv[i];
  }
  out << "\n\n"
      << "#include <ns2.hpp>\n\n"
      << "#include <vector>\n"
      << "#include <string>\n"
      << "#include <iomanip>\n"
      << "#include <istream>\n"
      << "#include <ostream>\n";
}

// ----------------------------------------------------------------------------

static void dump_json_struct(std::ostream *out_, tree_t const &tree,
                             std::string const &struct_name,
                             std::string const &indent = "") {
  std::ostream &out = *out_;
  out << indent << "struct " << struct_name << "_t {\n";
  for (size_t i = 0; i < tree.data.size(); i++) {
    tree_t::entry_t const &entry = tree.data[i];
    if (entry.subtree == NULL) {
      for (size_t j = 0; j < entry.keys.size(); j++) {
        token_t const &key = entry.keys[j];
        print(&out,
              "@  // @: @\n"
              "@  bool is_@_null;\n"
              "@  bool is_@_given;\n"
              "@  @ @;\n\n",
              indent, key.text, entry.type.text, indent, key.cpp_text, indent,
              key.cpp_text, indent, get_cpp_type(entry.type.text),
              key.cpp_text);
      }
    } else {
      std::string type("map" + ns2::to_string(i));
      dump_json_struct(&out, *entry.subtree, type, indent + "  ");
      out << "\n\n";
      out << indent << "  " << type << "_t";
      for (size_t j = 0; j < entry.keys.size(); j++) {
        token_t const &key = entry.keys[j];
        if (j > 0) {
          out << ",";
        }
        out << " " << key.cpp_text;
      }
      out << ";\n\n";
    }
  }
  out << indent << "};";
}

// ----------------------------------------------------------------------------

static std::string stringify(std::string const &s) {
  std::string ret("\"");
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '"') {
      ret += "\\\"";
    } else {
      ret += s[i];
    }
  }
  ret += "\"";
  return ret;
}

static void dump_json_write_rec(std::ostream *out_, tree_t const &tree,
                                std::string const &prefix, size_t indent,
                                size_t indent_step) {
  std::ostream &out = *out_;
  std::string indent_str(indent, ' ');
  out << "  out << \"" << indent_str << "{\\n\";\n\n";
  for (size_t i = 0; i < tree.data.size(); i++) {
    tree_t::entry_t const &entry = tree.data[i];
    if (entry.subtree == NULL) {
      for (size_t j = 0; j < entry.keys.size(); j++) {
        std::string coma;
        if (i + 1 < tree.data.size() || j + 1 < entry.keys.size()) {
          coma = ",";
        }
        token_t const &key = entry.keys[j];
        std::string indent_str(indent + indent_step, ' ');
        print(&out,
              "  // @: @\n"
              "  if (@.is_@_given) {\n"
              "    if (@.is_@_null) {\n"
              "      out << @ << \": null@\\n\";\n"
              "    } else {\n",
              key.text, entry.type.text, prefix, key.cpp_text, prefix,
              key.cpp_text, stringify(indent_str + key.text), coma);
        if (entry.type.text == "string") {
          print(&out, "      out << @ << \": \" << @.@ << \"@\\n\";\n",
                stringify(indent_str + key.text), prefix, key.cpp_text, coma);
        } else if (entry.type.text == "vector_string") {
          print(&out,
                "      out << @ << \": [\\n\";\n"
                "      std::vector<std::string> const &v = @.@;\n"
                "      for (size_t i = 0; i < v.size(); i++) {\n"
                "        out << \"@  \\\"\" << v[i] << \"\\\"\";\n"
                "        if (i + 1 < v.size()) {\n"
                "          out << \",\\n\";\n"
                "        } else {\n"
                "          out << \"\\n\";\n"
                "        }\n"
                "      }\n"
                "      out << \"@]@\\n\";\n",
                stringify(indent_str + key.text), prefix, key.cpp_text,
                indent_str, indent_str, coma);
        } else if (entry.type.text == "double") {
          print(&out,
                "      out << @ << \": \"\n"
                "          << std::setprecision(std::numeric_limits<\n"
                "                               double>::digits10 + 1)\n"
                "          << std::scientific << @.@ << \"@\\n\";\n",
                stringify(indent_str + key.text), prefix, key.cpp_text, coma);
        } else if (entry.type.text == "vector_double") {
          print(&out,
                "      out << @ << \": [\\n\";\n"
                "      std::vector<double> const &v = @.@;\n"
                "      for (size_t i = 0; i < v.size(); i++) {\n"
                "        out << \"@  \"\n"
                "            << std::setprecision(std::numeric_limits<\n"
                "                                 double>::digits10 + 1)\n"
                "            << std::scientific << v[i];\n"
                "        if (i + 1 < v.size()) {\n"
                "          out << \",\\n\";\n"
                "        } else {\n"
                "          out << \"\\n\";\n"
                "        }\n"
                "      }\n"
                "      out << \"@]@\\n\";\n",
                stringify(indent_str + key.text), prefix, key.cpp_text,
                indent_str, indent_str, coma);
        } else if (entry.type.text == "bool") {
          print(&out,
                "      out << @ << \": \"\n"
                "          << (@.@ ? \"true\" : \"false\") << \"@\\n\";\n",
                stringify(indent_str + key.text), prefix, key.cpp_text, coma);
        } else if (entry.type.text == "vector_bool") {
          print(&out,
                "      out << @ << \": [\\n\";\n"
                "      std::vector<bool> const &v = @.@;\n"
                "      for (size_t i = 0; i < v.size(); i++) {\n"
                "        out << \"@  \" <<\n"
                "            << (v[i] ? \"true\" : \"false\");\n"
                "        if (i + 1 < v.size()) {\n"
                "          out << \",\\n\";\n"
                "        } else {\n"
                "          out << \"\\n\";\n"
                "        }\n"
                "      }\n"
                "      out << \"@]@\\n\";\n",
                stringify(indent_str + key.text), prefix, key.cpp_text,
                indent_str, indent_str, coma);
        }
        print(&out, "    }\n"
                    "  }\n\n");
      }
    } else {
      for (size_t j = 0; j < entry.keys.size(); j++) {
        token_t const &key = entry.keys[j];
        out << "  // " << key.text << ": submap\n";
        dump_json_write_rec(&out, *entry.subtree, prefix + "." + key.cpp_text,
                            indent + indent_step, indent_step);
        if (j + 1 < entry.keys.size()) {
          out << "  out << \",\\n\";\n\n";
        } else {
          out << "  out << \"\\n\";\n\n";
        }
      }
    }
  }
  out << "  out << \"" << indent_str << "}\";\n\n";
}

static void dump_json_write(std::ostream *out_, tree_t const &tree,
                            std::string const &struct_name,
                            size_t indent_step) {
  std::ostream &out = *out_;
  out << "void write_" << struct_name << "(std::ostream *out_, " << struct_name
      << "_t const &" << struct_name << ") {\n"
      << "  std::ostream &out = *out_;\n";
  dump_json_write_rec(&out, tree, struct_name, 0, indent_step);
  out << "  out << \"\\n\";\n";
  out << "}";
}

// ----------------------------------------------------------------------------

struct key_desc_t {
  enum type_t {
    String,
    VectorString,
    Double,
    VectorDouble,
    Bool,
    VectorBool,
    Key
  } type;

  std::string class_id;
  std::string struct_id;
  std::string cpp_text;
  std::string key_id;
  std::string map_id;
  std::string bool_id;
  std::string enum_id;
  std::string cursor_id;
  std::string string_id;

  std::string type_to_string() const {
    switch(type) {
    case String:
      return std::string("string");
    case VectorString:
      return std::string("array of strings");
    case Double:
      return std::string("number");
    case VectorDouble:
      return std::string("array of numbers");
    case Bool:
      return std::string("boolean");
    case VectorBool:
      return std::string("array of booleans");
    case Key:
      return std::string("key");
    }
  }
};

typedef std::map<std::string, std::vector<key_desc_t> > maps_t;

static void list_ids(std::vector<key_desc_t> *ids_, maps_t *maps_,
                     tree_t const &tree, std::string const &class_prefix,
                     std::string const &struct_prefix,
                     std::string const &string_id_prefix) {
  std::vector<key_desc_t> &ids = *ids_;
  maps_t &maps = *maps_;
  std::string map_id(class_prefix);
  for (size_t i = 0; i < tree.data.size(); i++) {
    tree_t::entry_t const &entry = tree.data[i];
    if (entry.subtree == NULL) {
      for (size_t j = 0; j < entry.keys.size(); j++) {
        token_t const &key = entry.keys[j];
        key_desc_t kd;
        kd.class_id = class_prefix + "_" + key.cpp_text;
        kd.struct_id = struct_prefix;
        kd.cpp_text = key.cpp_text;
        kd.map_id = map_id;
        kd.key_id = key.text;
        if (entry.type.text == "double") {
          kd.type = key_desc_t::Double;
        } else if (entry.type.text == "vector_double") {
          kd.type = key_desc_t::VectorDouble;
        } else if (entry.type.text == "string") {
          kd.type = key_desc_t::String;
        } else if (entry.type.text == "vector_string") {
          kd.type = key_desc_t::VectorString;
        } else if (entry.type.text == "bool") {
          kd.type = key_desc_t::Bool;
        } else if (entry.type.text == "vector_bool") {
          kd.type = key_desc_t::VectorBool;
        } else {
          abort(); // should never happen
        }
        kd.bool_id = "is_" + kd.class_id + "_filled";
        if (kd.type == key_desc_t::VectorDouble ||
            kd.type == key_desc_t::VectorString) {
          kd.enum_id = "waiting_for_next_" + kd.class_id + "_value";
        } else {
          kd.enum_id = "waiting_for_" + kd.class_id + "_value";
        }
        kd.cursor_id = "cursor_of_" + kd.class_id;
        kd.string_id = stringify(string_id_prefix + ":" + key.text);
        ids.push_back(kd);
        maps[kd.map_id].push_back(kd);
      }
    } else {
      for (size_t j = 0; j < entry.keys.size(); j++) {
        token_t const &key = entry.keys[j];
        key_desc_t kd;
        kd.key_id = key.text;
        kd.type = key_desc_t::Key;
        kd.map_id = map_id;
        kd.enum_id = class_prefix + "_" + key.cpp_text;
        maps[kd.map_id].push_back(kd);
        list_ids(
            &ids, &maps, *entry.subtree, class_prefix + "_" + key.cpp_text,
            (struct_prefix.size() > 0 ? struct_prefix + "." + key.cpp_text
                                      : key.cpp_text),
            (string_id_prefix.size() > 0 ? string_id_prefix + ":" + key.text
                                         : key.text));
      }
    }
  }
}

// ----------------------------------------------------------------------------

static void dump_json_read(std::ostream *out_, tree_t const &tree,
                           std::string const &class_name,
                           std::string const &struct_name) {
  std::ostream &out = *out_;
  std::vector<key_desc_t> kds;
  maps_t maps;

  list_ids(&kds, &maps, tree, "root", "", "");

  // beginning of class
  print(&out,
        "struct @ : public ns2::json_parser_t {\n\n"
        "  @_t *buf;\n\n"
        "  bool inside_array;\n\n",
        class_name, struct_name);

  // enum
  print(&out, "  enum state_t {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    waiting_for_a_key_in_@,\n", it->first);
  }
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out, "    @@\n", kds[i].enum_id,
          std::string(i + 1 < kds.size() ? "," : ""));
  }
  print(&out, "  } state;\n\n"
              "  std::vector<state_t> nested_maps;\n\n");

  // bools and cursors
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out,
          "  bool @;\n"
          "  ns2::cursor_t @;\n\n",
          kds[i].bool_id, kds[i].cursor_id);
  }

  // ctor
  print(&out, "  @(@_t *buf_) {\n", class_name, struct_name);
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out,
          "    @ = false;\n"
          "    buf_->%.is_@_given = false;\n",
          kds[i].bool_id, kds[i].struct_id, kds[i].cpp_text);
  }
  print(&out, "    state = waiting_for_a_key_in_root;\n"
              "    nested_maps.push_back(waiting_for_a_key_in_root);\n"
              "    buf = buf_;\n"
              "    inside_array = false;\n"
              "  }\n\n");

  // implementation of exception for wrong type
  print(&out, "  void throw_wrong_type(std::string const &expected_type,\n"
              "                        std::string const &key,\n"
              "                        ns2::cursor_t const &cursor) const {\n"
              "    std::string msg(\"ERROR: expecting value of type\" +\n"
              "                    expected_type + \" for key \" + key +\n"
              "                    \" at \" + cursor.to_string());\n"
              "    NS2_THROW(std::runtime_error, msg.c_str());\n"
              "  }\n\n");

  // implementation of begin_map
  print(&out, "  bool begin_map(ns2::cursor_t const &cursor) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
  }
  print(&out, "      return true; // nothing to do\n");
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out,
          "    case @:\n"
          "      throw_wrong_type(\"@\", @, cursor);\n"
          "      return false;\n",
          kds[i].enum_id, kds[i].type_to_string(), kds[i].string_id);
  }
  print(&out, "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of end_map
  print(&out, "  bool end_map(ns2::cursor_t const &cursor) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
    //for (size_t i = 0; i < it->second.size(); i++) {
    //  key_desc_t const &kd = it->second[i];
    //  if (kd.type == key_desc_t::Key) {
    //    continue;
    //  }
    //  print(&out,
    //        "      if (!@) {\n"
    //        "        std::string msg(\"key \"@\" not provided\");\n"
    //        "        NS2_THROW(std::runtime_error, msg.c_str());\n"
    //        "        return false;\n"
    //        "      }\n",
    //        kd.bool_id, kd.string_id);
    //}
  }
  print(&out, "      nested_maps.pop_back();\n"
              "      state = nested_maps.back();\n"
              "      return true;\n");
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out,
          "    case @:\n"
          "      throw_wrong_type(\"@\", @, cursor);\n"
          "      return false;\n",
          kds[i].enum_id, kds[i].type_to_string(), kds[i].string_id);
  }
  print(&out, "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of begin_array
  print(&out, "  std::vector<double> vector_double_buf;\n"
              "  std::vector<std::string> vector_string_buf;\n\n"
              "  bool begin_array(ns2::cursor_t const &cursor) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
  }
  print(&out, "    {\n"
              "      std::string msg(\"ERROR: expected key at \" +\n"
              "                      cursor.to_string());\n"
              "      NS2_THROW(std::runtime_error, msg.c_str());\n"
              "      return false;\n"
              "    }\n");
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out, "    case @:\n", kds[i].enum_id);
    if (kds[i].type == key_desc_t::VectorDouble ||
        kds[i].type == key_desc_t::VectorString) {
      print(&out, "      inside_array = true;\n"
                  "      return true; // nothing to do\n");
    } else {
      print(&out,
            "      throw_wrong_type(\"@\", @, cursor);\n"
            "      return false;\n",
            kds[i].type_to_string(), kds[i].string_id);
    }
  }
  print(&out, "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of end_array
  print(&out, "  bool end_array(ns2::cursor_t const &cursor) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
  }
  print(&out, "    {\n"
              "      std::string msg(\"ERROR: expected key at \" +\n"
              "                      cursor.to_string());\n"
              "      NS2_THROW(std::runtime_error, msg.c_str());\n"
              "      return false;\n"
              "    }\n");
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out, "    case @:\n", kds[i].enum_id);
    if (kds[i].type == key_desc_t::VectorDouble) {
      print(&out,
            "      buf->%.@ = vector_double_buf;\n"
            "      buf->%.is_@_null = false;\n"
            "      inside_array = false;\n"
            "      state = nested_maps.back();\n"
            "      return true;\n",
            kds[i].struct_id, kds[i].cpp_text, kds[i].struct_id,
            kds[i].cpp_text);
    } else if (kds[i].type == key_desc_t::VectorString) {
      print(&out,
            "      buf->%.@ = vector_string_buf;\n"
            "      buf->%.is_@_null = false;\n"
            "      inside_array = false;\n"
            "      state = nested_maps.back();\n"
            "      return true;\n",
            kds[i].struct_id, kds[i].cpp_text, kds[i].struct_id,
            kds[i].cpp_text);
    } else {
      print(&out,
            "      throw_wrong_type(\"@\", @, cursor);\n"
            "      return false;\n",
            kds[i].type_to_string(), kds[i].string_id);
    }
  }
  print(&out, "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of new_key
  print(&out, "  bool new_key(ns2::cursor_t const &cursor,\n"
              "               std::string const &key) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out,
          "    case waiting_for_a_key_in_@:\n"
          "      ",
          it->first);
    for (size_t i = 0; i < it->second.size(); i++) {
      key_desc_t const &kd = it->second[i];
      print(&out, "if (key == @) {\n", kd.key_id);
      if (kd.type == key_desc_t::Key) {
        print(&out,
              "        state = waiting_for_a_key_in_@;\n"
              "        nested_maps.push_back(state);\n"
              "        return true;\n",
              kd.enum_id);
      } else {
        print(&out,
              "        if (@) {\n"
              "          std::string msg(\"ERROR: key \" @ \n"
              "                          \" already given at \" +\n"
              "                          cursor.to_string());\n"
              "          NS2_THROW(std::runtime_error, msg.c_str());\n"
              "        }\n"
              "        @ = true;\n"
              "        buf->%.is_@_given = true;\n"
              "        @ = cursor;\n"
              "        state = @;\n",
              kd.bool_id, kd.string_id, kd.bool_id, kd.struct_id, kd.cpp_text,
              kd.cursor_id, kd.enum_id);
        if (kd.type == key_desc_t::VectorDouble) {
          print(&out, "        vector_double_buf.clear();\n");
        } else if (kd.type == key_desc_t::VectorString) {
          print(&out, "        vector_string_buf.clear();\n");
        }
        print(&out, "        return true;\n");
      }
      print(&out, "      } else ");
    }
    print(&out, "{\n"
                "        std::string msg(\"ERROR: unknown key '\" + key +\n"
                "                        \"' at \" + cursor.to_string());\n"
                "        NS2_THROW(std::runtime_error, msg.c_str());\n"
                "        return false;\n"
                "      }\n");
  }
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out, "    case @:\n", kds[i].enum_id);
  }
  print(&out, "      {\n"
              "        std::string msg(\"unexpected key at \" +\n"
              "                        cursor.to_string());\n"
              "        NS2_THROW(std::runtime_error, msg.c_str());\n"
              "        return false;\n"
              "      }\n"
              "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of new_null
  print(&out, "  bool new_null(ns2::cursor_t const &cursor) {\n"
              "    switch(state) {\n");
  for (size_t i = 0; i < kds.size(); i++) {
    print(&out, "    case @:\n"
                "      buf->%.is_@_null = true;\n"
                "      state = nested_maps.back();\n"
                "      return true;\n",
                kds[i].enum_id, kds[i].struct_id, kds[i].cpp_text);
  }
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
  }
  print(&out, "      {\n"
              "        std::string msg(\"ERROR: unexpected null at\" +\n"
              "                        cursor.to_string());\n"
              "        NS2_THROW(std::runtime_error, msg.c_str());\n"
              "        return false;\n"
              "      }\n"
              "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of new_bool
  print(&out, "  bool new_boolean(ns2::cursor_t const &cursor, bool b) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
  }
  print(&out, "      {\n"
              "        std::string msg(\"ERROR: unexpected boolean at \" +\n"
              "                        cursor.to_string());\n"
              "        NS2_THROW(std::runtime_error, msg.c_str());\n"
              "        return false;\n"
              "      }\n");
  for (size_t i = 0; i < kds.size(); i++) {
    out << "    case " << kds[i].enum_id << ":\n";
    switch(kds[i].type) {
    case key_desc_t::Key:
      abort(); // should never happen
      break;
    case key_desc_t::VectorDouble:
    case key_desc_t::Double:
    case key_desc_t::VectorString:
    case key_desc_t::String:
      print(&out,
            "      throw_wrong_type(\"@\", @, cursor);\n"
            "      return false;\n",
            kds[i].type_to_string(), kds[i].string_id);
      break;
    case key_desc_t::Bool:
      print(&out,
            "      buf->%.@ = b;\n"
            "      buf->%.is_@_null = false;\n"
            "      state = nested_maps.back();\n"
            "      return true;\n",
            kds[i].struct_id, kds[i].cpp_text, kds[i].struct_id,
            kds[i].cpp_text);
      break;
    case key_desc_t::VectorBool:
      print(&out,
            "      if (!inside_array) {\n"
            "        throw_wrong_type(\"@\", @, cursor);\n"
            "        return false;\n"
            "      }\n"
            "      vector_string_buf.push_back(b);\n"
            "      return true;\n",
            kds[i].type_to_string(), kds[i].string_id);
      break;
    }
  }
  print(&out, "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of new_string
  print(&out, "  bool new_string(ns2::cursor_t const &cursor,\n"
              "                  std::string const &s) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
  }
  print(&out, "      {\n"
              "        std::string msg(\"ERROR: unexpected string at \" +\n"
              "                        cursor.to_string());\n"
              "        NS2_THROW(std::runtime_error, msg.c_str());\n"
              "        return false;\n"
              "      }\n");
  for (size_t i = 0; i < kds.size(); i++) {
    out << "    case " << kds[i].enum_id << ":\n";
    switch(kds[i].type) {
    case key_desc_t::Key:
      abort(); // should never happen
      break;
    case key_desc_t::VectorDouble:
    case key_desc_t::Double:
    case key_desc_t::VectorBool:
    case key_desc_t::Bool:
      print(&out,
            "      throw_wrong_type(\"@\", @, cursor);\n"
            "      return false;\n",
            kds[i].type_to_string(), kds[i].string_id);
      break;
    case key_desc_t::String:
      print(&out,
            "      buf->%.@ = s;\n"
            "      buf->%.is_@_null = false;\n"
            "      state = nested_maps.back();\n"
            "      return true;\n",
            kds[i].struct_id, kds[i].cpp_text, kds[i].struct_id,
            kds[i].cpp_text);
      break;
    case key_desc_t::VectorString:
      print(&out,
            "      if (!inside_array) {\n"
            "        throw_wrong_type(\"@\", @, cursor);\n"
            "        return false;\n"
            "      }\n"
            "      vector_string_buf.push_back(s);\n"
            "      return true;\n",
            kds[i].type_to_string(), kds[i].string_id);
      break;
    }
  }
  print(&out, "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // implementation of new_number
  print(&out, "  bool new_number(ns2::cursor_t const &cursor, double d) {\n"
              "    switch(state) {\n");
  for (maps_t::const_iterator it = maps.begin(); it != maps.end(); ++it) {
    print(&out, "    case waiting_for_a_key_in_@:\n", it->first);
  }
  print(&out, "      {\n"
              "        std::string msg(\"ERROR: unexpected number at \" +\n"
              "                        cursor.to_string());\n"
              "        NS2_THROW(std::runtime_error, msg.c_str());\n"
              "        return false;\n"
              "      }\n");
  for (size_t i = 0; i < kds.size(); i++) {
    out << "    case " << kds[i].enum_id << ":\n";
    switch(kds[i].type) {
    case key_desc_t::Key:
      abort(); // should never happen
      break;
    case key_desc_t::VectorString:
    case key_desc_t::String:
    case key_desc_t::VectorBool:
    case key_desc_t::Bool:
      print(&out,
            "      throw_wrong_type(\"@\", @, cursor);\n"
            "      return false;\n",
            kds[i].type_to_string(), kds[i].string_id);
      break;
    case key_desc_t::Double:
      print(&out,
            "      buf->%.@ = d;\n"
            "      buf->%.is_@_null = false;\n"
            "      state = nested_maps.back();\n"
            "      return true;\n",
            kds[i].struct_id, kds[i].cpp_text, kds[i].struct_id,
            kds[i].cpp_text);
      break;
    case key_desc_t::VectorDouble:
      print(&out,
            "      if (!inside_array) {\n"
            "        throw_wrong_type(\"@\", @, cursor);\n"
            "        return false;\n"
            "      }\n"
            "      vector_double_buf.push_back(d);\n"
            "      return true;\n",
            kds[i].type_to_string(), kds[i].string_id);
      break;
    }
  }
  print(&out, "    }\n"
              "    return true; // silence warning\n"
              "  }\n\n");

  // end of class and read function
  print(&out,
        "};\n\n"
        "void read_@(std::istream *in_, @_t *buf) {\n"
        "  std::istream &in = *in_;\n"
        "  @ parser(buf);\n"
        "  std::vector<std::string> errors;\n"
        "  ns2::parse_json(&in, &errors, &parser);\n"
        "  if (errors.size() > 0) {\n"
        "    NS2_THROW(std::runtime_error, errors[0].c_str());\n"
        "  }\n"
        "}\n",
        struct_name, struct_name, class_name);
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "--help")) {
    std::cout << argv[0] << ": usage: " << argv[0] << " file.jsonc\n";
    return 0;
  }

  std::string class_name("json_parser"), struct_name("json"),
      clang_format("clang-format"), filename;

  for (int i = 1; i < argc; i++) {
    if (!memcmp(argv[i], "--class-name=", 13)) {
      class_name = argv[i] + 13;
      continue;
    } else if (!memcmp(argv[i], "--struct-name=", 14)) {
      struct_name = argv[i] + 14;
      continue;
    } else if (!memcmp(argv[i], "--clang-format=", 15)) {
      clang_format = argv[i] + 15;
      continue;
    } else {
      if (filename.size() > 0) {
        std::cerr << "ERROR: input filename already given: " << filename
                  << std::endl;
        return -1;
      }
      filename = argv[i];
      continue;
    }
  }

  if (filename.size() == 0) {
    std::cerr << "ERROR: no filename given" << std::endl;
    return -1;
  }

  ns2::ifile_t in(filename);
  std::vector<token_t> tokens;
  std::string error;

  tokenize(&tokens, &in, &error);
  if (error.size() > 0) {
    std::cerr << argv[0] << ": ERROR: " << error << "\n";
    return -1;
  }

  cppize_tokens(&tokens);

  tree_t tree;
  parse(&tree, &error, tokens);
  if (error.size() > 0) {
    std::cerr << argv[0] << ": ERROR: " << error << "\n";
    return -1;
  }

  dump_header(&std::cout, argc, argv);
  dump_sep(&std::cout);
  dump_json_struct(&std::cout, tree, struct_name);
  dump_sep(&std::cout);
  dump_json_write(&std::cout, tree, struct_name, 2);
  dump_sep(&std::cout);
  dump_json_read(&std::cout, tree, class_name, struct_name);

  return 0;
}

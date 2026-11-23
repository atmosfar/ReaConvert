/*
 * tiny_json.h -- a deliberately tiny JSON reader/writer helper.
 *
 * Part of the ReaConvert "JSON timeline" example extension.
 * This is NOT a production JSON library: it supports just enough of RFC 8259
 * to read and write the example schema (objects, arrays, strings, numbers,
 * booleans and null). No exceptions, no allocations tricks, no comments.
 *
 * Public domain / do whatever you want with it.
 */

#ifndef TINY_JSON_H
#define TINY_JSON_H

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace tinyjson {

struct Value;
typedef std::vector<Value> Array;
// vector of pairs instead of a map, so the key order of the source file survives
typedef std::vector<std::pair<std::string, Value> > Object;

struct Value {
  enum Type { Null, True, False, Number, String, Array_, Object_ };

  Type type;
  double num;
  std::string str;
  Array arr;
  Object obj;

  Value() : type(Null), num(0.0) {}

  bool isNull() const { return type == Null; }
  bool isBool() const { return type == True || type == False; }
  bool isNumber() const { return type == Number; }
  bool isString() const { return type == String; }
  bool isArray() const { return type == Array_; }
  bool isObject() const { return type == Object_; }

  // accessors with defaults, so a schema field can simply be missing
  double asNumber(double dflt = 0.0) const {
    if (type == Number) return num;
    if (type == True) return 1.0;
    if (type == False) return 0.0;
    return dflt;
  }
  int asInt(int dflt = 0) const { return (int)asNumber((double)dflt); }
  bool asBool(bool dflt = false) const {
    if (type == True) return true;
    if (type == False) return false;
    if (type == Number) return num != 0.0;
    return dflt;
  }
  std::string asString(const std::string &dflt = std::string()) const {
    return type == String ? str : dflt;
  }

  size_t size() const {
    if (type == Array_) return arr.size();
    if (type == Object_) return obj.size();
    return 0;
  }

  // object member lookup, NULL when absent
  const Value *find(const char *key) const {
    if (type != Object_ || !key) return NULL;
    for (size_t i = 0; i < obj.size(); i++)
      if (obj[i].first == key) return &obj[i].second;
    return NULL;
  }
  // convenience: never returns NULL, returns a static null value instead
  const Value &member(const char *key) const {
    static const Value s_null;
    const Value *v = find(key);
    return v ? *v : s_null;
  }
  // array element lookup
  const Value &element(size_t i) const {
    static const Value s_null;
    return (type == Array_ && i < arr.size()) ? arr[i] : s_null;
  }
};

// ---------------------------------------------------------------- parsing ---

class Parser {
public:
  Parser(const std::string &text) : m_text(text), m_pos(0) {}

  bool parse(Value &out, std::string &err) {
    skipWS();
    if (!parseValue(out)) {
      err = errorMessage();
      return false;
    }
    skipWS();
    if (m_pos != m_text.size()) {
      err = errorMessage("trailing characters after top-level value");
      return false;
    }
    return true;
  }

private:
  const std::string &m_text;
  size_t m_pos;
  size_t m_errPos;
  std::string m_errMsg;

  bool atEnd() const { return m_pos >= m_text.size(); }
  char peek() const { return atEnd() ? '\0' : m_text[m_pos]; }
  void fail(const char *msg) {
    if (m_errMsg.empty()) {
      m_errPos = m_pos;
      m_errMsg = msg;
    }
  }
  std::string errorMessage(const char *extra = NULL) const {
    char buf[160];
    size_t line = 1, col = 1;
    for (size_t i = 0; i < m_errPos && i < m_text.size(); i++) {
      if (m_text[i] == '\n') {
        line++;
        col = 1;
      } else
        col++;
    }
    snprintf(buf, sizeof(buf), "JSON error at line %d column %d: %s%s%s",
             (int)line, (int)col, m_errMsg.empty() ? "unexpected end of input" : m_errMsg.c_str(),
             extra ? " (" : "", extra ? extra : "");
    if (extra) {
      size_t l = strlen(buf);
      if (l + 2 < sizeof(buf)) {
        buf[l++] = ')';
        buf[l] = 0;
      }
    }
    return std::string(buf);
  }

  void skipWS() {
    while (!atEnd()) {
      char c = peek();
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        m_pos++;
      else
        break;
    }
  }

  bool literal(const char *s, Value &out, Value::Type t, double n = 0.0) {
    size_t l = strlen(s);
    if (m_text.compare(m_pos, l, s) != 0) {
      fail("malformed literal");
      return false;
    }
    m_pos += l;
    out.type = t;
    out.num = n;
    return true;
  }

  bool parseValue(Value &out) {
    skipWS();
    if (atEnd()) {
      fail("unexpected end of input");
      return false;
    }
    char c = peek();
    switch (c) {
      case '{': return parseObject(out);
      case '[': return parseArray(out);
      case '"': {
        out.type = Value::String;
        return parseString(out.str);
      }
      case 't': return literal("true", out, Value::True, 1.0);
      case 'f': return literal("false", out, Value::False, 0.0);
      case 'n': return literal("null", out, Value::Null, 0.0);
      default: return parseNumber(out);
    }
  }

  bool parseNumber(Value &out) {
    const char *start = m_text.c_str() + m_pos;
    char *endp = NULL;
    double d = strtod(start, &endp);
    if (endp == start) {
      fail("expected a value");
      return false;
    }
    m_pos += (size_t)(endp - start);
    out.type = Value::Number;
    out.num = d;
    return true;
  }

  bool parseString(std::string &out) {
    if (peek() != '"') {
      fail("expected '\"'");
      return false;
    }
    m_pos++;
    out.clear();
    for (;;) {
      if (atEnd()) {
        fail("unterminated string");
        return false;
      }
      char c = m_text[m_pos++];
      if (c == '"') return true;
      if (c != '\\') {
        out += c;
        continue;
      }
      if (atEnd()) {
        fail("unterminated escape");
        return false;
      }
      char e = m_text[m_pos++];
      switch (e) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          unsigned int cp = 0;
          for (int i = 0; i < 4; i++) {
            if (atEnd()) {
              fail("truncated \\u escape");
              return false;
            }
            char h = m_text[m_pos++];
            cp <<= 4;
            if (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
            else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
            else {
              fail("bad hex digit in \\u escape");
              return false;
            }
          }
          appendUTF8(out, cp);
          break;
        }
        default:
          fail("unknown escape sequence");
          return false;
      }
    }
  }

  static void appendUTF8(std::string &out, unsigned int cp) {
    if (cp < 0x80) {
      out += (char)cp;
    } else if (cp < 0x800) {
      out += (char)(0xC0 | (cp >> 6));
      out += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += (char)(0xE0 | (cp >> 12));
      out += (char)(0x80 | ((cp >> 6) & 0x3F));
      out += (char)(0x80 | (cp & 0x3F));
    } else {
      out += (char)(0xF0 | (cp >> 18));
      out += (char)(0x80 | ((cp >> 12) & 0x3F));
      out += (char)(0x80 | ((cp >> 6) & 0x3F));
      out += (char)(0x80 | (cp & 0x3F));
    }
  }

  bool parseArray(Value &out) {
    out.type = Value::Array_;
    m_pos++;  // '['
    skipWS();
    if (peek() == ']') {
      m_pos++;
      return true;
    }
    for (;;) {
      Value v;
      if (!parseValue(v)) return false;
      out.arr.push_back(v);
      skipWS();
      if (atEnd()) {
        fail("unterminated array");
        return false;
      }
      char c = m_text[m_pos++];
      if (c == ',') continue;
      if (c == ']') return true;
      m_pos--;
      fail("expected ',' or ']'");
      return false;
    }
  }

  bool parseObject(Value &out) {
    out.type = Value::Object_;
    m_pos++;  // '{'
    skipWS();
    if (peek() == '}') {
      m_pos++;
      return true;
    }
    for (;;) {
      skipWS();
      std::string key;
      if (!parseString(key)) return false;
      skipWS();
      if (peek() != ':') {
        fail("expected ':'");
        return false;
      }
      m_pos++;
      Value v;
      if (!parseValue(v)) return false;
      out.obj.push_back(std::make_pair(key, v));
      skipWS();
      if (atEnd()) {
        fail("unterminated object");
        return false;
      }
      char c = m_text[m_pos++];
      if (c == ',') continue;
      if (c == '}') return true;
      m_pos--;
      fail("expected ',' or '}'");
      return false;
    }
  }
};

// parse a whole document; returns false and fills err on failure
inline bool parse(const std::string &text, Value &out, std::string &err) {
  Parser p(text);
  return p.parse(out, err);
}

// read and parse a file
inline bool parseFile(const std::string &path, Value &out, std::string &err) {
  FILE *fp = fopen(path.c_str(), "rb");
  if (!fp) {
    err = "could not open '" + path + "'";
    return false;
  }
  std::string text;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) text.append(buf, n);
  fclose(fp);
  return parse(text, out, err);
}

// ---------------------------------------------------------------- writing ---

inline std::string escape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (size_t i = 0; i < s.size(); i++) {
    unsigned char c = (unsigned char)s[i];
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char hex[8];
          snprintf(hex, sizeof(hex), "\\u%04x", c);
          out += hex;
        } else {
          out += (char)c;
        }
    }
  }
  return out;
}

inline std::string quoted(const std::string &s) {
  return "\"" + escape(s) + "\"";
}

// a small hand-built JSON emitter (this example only ever writes its own
// schema, so a full writer was not worth it)
class Writer {
public:
  Writer() : m_indent(0) {}

  void line(const char *fmt, ...) {
    std::string pad(m_indent * 2, ' ');
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    m_out += pad;
    m_out += buf;
    m_out += "\n";
  }
  void openLine(const std::string &s) {
    line("%s", s.c_str());
    m_indent++;
  }
  void closeLine(const std::string &s) {
    if (m_indent > 0) m_indent--;
    line("%s", s.c_str());
  }

  const std::string &str() const { return m_out; }

private:
  std::string m_out;
  size_t m_indent;
};

}  // namespace tinyjson

#endif  // TINY_JSON_H
